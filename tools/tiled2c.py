#!/usr/bin/env python3
"""
Tiled .tmx -> C code converter
Usage: python tiled2c.py <map.tmx> <map_data.c> <map_data.h>

Auto-discovers all tile custom properties from .tsx and generates:
- bool   -> uint32_t bitmask + TILE_HAS_XXX(tile_idx) macro
- int    -> int32_t array    + TILE_GET_XXX(tile_idx) macro
- float  -> float array      + TILE_GET_XXX(tile_idx) macro

Also exports layer data + object layer data.
"""
import xml.etree.ElementTree as ET
import sys
import os
import re


def safe_c_name(s):
    """Convert string to valid C identifier."""
    s = re.sub(r'[^a-zA-Z0-9_]', '_', s)
    if s and s[0].isdigit():
        s = '_' + s
    return s.lower() if s else 'unnamed'


def parse_properties(elem):
    """Parse Tiled <properties> element, return {name: (type, value)} dict."""
    props = {}
    props_elem = elem.find('properties')
    if props_elem is None:
        return props
    for p in props_elem.findall('property'):
        name = p.attrib.get('name', '')
        ptype = p.attrib.get('type', 'string')
        value = p.attrib.get('value', '')
        if ptype == 'bool':
            props[name] = ('bool', value.lower() == 'true')
        elif ptype == 'int':
            props[name] = ('int', int(value) if value else 0)
        elif ptype == 'float':
            props[name] = ('float', float(value) if value else 0.0)
    return props


def discover_tile_properties(root):
    """Scan all tiles, collect property-name -> type mapping."""
    prop_types = {}
    for tile in root.findall('tile'):
        props = parse_properties(tile)
        for name, (ptype, _) in props.items():
            if name not in prop_types:
                prop_types[name] = ptype
            elif prop_types[name] != ptype:
                print(f"Warning: property '{name}' has conflicting types, using first seen", file=sys.stderr)
    return prop_types


def build_tile_property_arrays(root, prop_types, tilecount):
    """Build full arrays (tilecount elements) for each property."""
    bool_arrays = {name: [False] * tilecount for name, t in prop_types.items() if t == 'bool'}
    int_arrays = {name: [0] * tilecount for name, t in prop_types.items() if t == 'int'}
    float_arrays = {name: [0.0] * tilecount for name, t in prop_types.items() if t == 'float'}

    for tile in root.findall('tile'):
        tile_id = int(tile.attrib.get('id', 0))
        if tile_id < 0 or tile_id >= tilecount:
            continue
        props = parse_properties(tile)
        for name, (ptype, value) in props.items():
            if ptype == 'bool' and name in bool_arrays:
                bool_arrays[name][tile_id] = value
            elif ptype == 'int' and name in int_arrays:
                int_arrays[name][tile_id] = value
            elif ptype == 'float' and name in float_arrays:
                float_arrays[name][tile_id] = value

    return bool_arrays, int_arrays, float_arrays


def write_bool_bitmask(fc, name, values):
    """Write bool bitmask as uint32_t array."""
    word_count = (len(values) + 31) // 32
    words = [0] * word_count
    for i, v in enumerate(values):
        if v:
            words[i >> 5] |= (1 << (i & 0x1F))
    fc.write(f'const uint32_t tileprop_{name}[{word_count}] = {{\n')
    for i in range(0, len(words), 8):
        chunk = words[i:i + 8]
        fc.write('    ' + ', '.join(f'0x{w:08X}U' for w in chunk) + ',\n')
    fc.write('};\n\n')


def write_int_array(fc, name, values):
    """Write int32_t array."""
    fc.write(f'const int32_t tileprop_{name}[{len(values)}] = {{\n')
    for i in range(0, len(values), 16):
        chunk = values[i:i + 16]
        fc.write('    ' + ', '.join(str(v) for v in chunk) + ',\n')
    fc.write('};\n\n')


def write_float_array(fc, name, values):
    """Write float array."""
    fc.write(f'const float tileprop_{name}[{len(values)}] = {{\n')
    for i in range(0, len(values), 8):
        chunk = values[i:i + 8]
        parts = []
        for v in chunk:
            if v == int(v) and abs(v) < 1e6:
                parts.append(f'{v:.1f}f')
            else:
                parts.append(f'{v!r}f')
        fc.write('    ' + ', '.join(parts) + ',\n')
    fc.write('};\n\n')


def parse_layer_data(layer_elem):
    """Extract CSV layer data, return list of ints."""
    data_elem = layer_elem.find('data')
    if data_elem is None:
        return None
    encoding = data_elem.attrib.get('encoding', 'csv')
    if encoding != 'csv':
        return None
    text = data_elem.text or ''
    text = text.strip().replace('\n', '').replace('\r', '').replace(' ', '')
    result = []
    for s in text.split(','):
        s = s.strip()
        if s:
            result.append(int(s) - 1)  # Tiled GID 1-based → 0-based
        else:
            result.append(-1)           # empty tile
    return result


def parse_objects(root):
    """Parse all objectgroups, return {type: [obj_dict, ...]} grouped by type."""
    groups = {}
    for og in root.findall('objectgroup'):
        for obj in og.findall('object'):
            oid = int(obj.attrib.get('id', 0))
            otype = obj.attrib.get('type', '')
            oname = obj.attrib.get('name', '')
            ox = float(obj.attrib.get('x', 0))
            oy = float(obj.attrib.get('y', 0))
            ow = float(obj.attrib.get('width', 0))
            oh = float(obj.attrib.get('height', 0))
            if not otype:
                continue
            props = parse_properties(obj)
            if otype not in groups:
                groups[otype] = []
            groups[otype].append({
                'id': oid,
                'name': oname[:31] if oname else '',
                'x': ox, 'y': oy,
                'w': ow, 'h': oh,
                'props': props,
            })
    return groups


def write_object_data(fc, groups):
    """Write object struct arrays and per-object property arrays."""
    for otype, objs in groups.items():
        ctype = safe_c_name(otype)
        count = len(objs)

        fc.write(f'// === object group: "{otype}" ===\n')
        fc.write(f'const map_obj_t map_obj_{ctype}[{count}] = {{\n')
        for obj in objs:
            fc.write(f'    {{{obj["id"]}, {obj["x"]:.1f}f, {obj["y"]:.1f}f, '
                     f'{obj["w"]:.1f}f, {obj["h"]:.1f}f, "{obj["name"]}"}},\n')
        fc.write('};\n\n')

        obj_prop_types = {}
        for obj in objs:
            for name, (ptype, _) in obj['props'].items():
                if name not in obj_prop_types:
                    obj_prop_types[name] = ptype

        for pname, ptype in obj_prop_types.items():
            cpname = safe_c_name(pname)
            if ptype == 'bool':
                word_count = (count + 31) // 32
                words = [0] * word_count
                for i, obj in enumerate(objs):
                    if pname in obj['props'] and obj['props'][pname][1]:
                        words[i >> 5] |= (1 << (i & 0x1F))
                fc.write(f'const uint32_t obj_{ctype}_{cpname}[{word_count}] = {{\n    ')
                fc.write(', '.join(f'0x{w:08X}U' for w in words))
                fc.write('\n};\n\n')
            elif ptype == 'int':
                vals = [obj['props'].get(pname, ('int', 0))[1] for obj in objs]
                fc.write(f'const int32_t obj_{ctype}_{cpname}[{count}] = {{\n    ')
                fc.write(', '.join(str(v) for v in vals))
                fc.write('\n};\n\n')
            elif ptype == 'float':
                vals = [obj['props'].get(pname, ('float', 0.0))[1] for obj in objs]
                parts = []
                for v in vals:
                    if v == int(v) and abs(v) < 1e6:
                        parts.append(f'{v:.1f}f')
                    else:
                        parts.append(f'{v!r}f')
                fc.write(f'const float obj_{ctype}_{cpname}[{count}] = {{\n    ')
                fc.write(', '.join(parts))
                fc.write('\n};\n\n')


def write_header(fh, basename, map_w, map_h, tile_w, tile_h, tilesets,
                 layers, bool_arrays, int_arrays, float_arrays, obj_groups):
    """Generate map_data.h."""
    guard = os.path.splitext(basename)[0].upper() + '_H'

    fh.write('/* Auto-generated by tiled2c.py -- DO NOT EDIT */\n')
    fh.write(f'#ifndef {guard}\n')
    fh.write(f'#define {guard}\n\n')
    fh.write('#include <stdint.h>\n')
    fh.write('#include <stdbool.h>\n')
    fh.write('#include "gamelib.h"\n\n')

    fh.write('/* === Map metadata === */\n')
    fh.write(f'#define MAP_WIDTH        {map_w}\n')
    fh.write(f'#define MAP_HEIGHT       {map_h}\n')
    fh.write(f'#define MAP_TILE_WIDTH   {tile_w}\n')
    fh.write(f'#define MAP_TILE_HEIGHT  {tile_h}\n\n')

    for ts in tilesets:
        tsname = safe_c_name(ts["name"]).upper()
        fh.write(f'/* tileset "{ts["name"]}" */\n')
        fh.write(f'#define MAP_TILESET_{tsname}_FIRSTGID  {ts["firstgid"]}\n')
        fh.write(f'#define MAP_TILESET_{tsname}_TILECOUNT  {ts["tilecount"]}\n')
        fh.write(f'#define MAP_TILESET_{tsname}_COLUMNS    {ts["columns"]}\n\n')

    fh.write('/* === Layer data === */\n')
    for lyr in layers:
        fh.write(f'extern const int16_t map_layer_{lyr["cname"]}[MAP_WIDTH * MAP_HEIGHT];\n')
    fh.write('\n')

    if bool_arrays or int_arrays or float_arrays:
        fh.write('/* === Tile property bitmasks/arrays === */\n')

        for name in sorted(bool_arrays.keys()):
            word_count = (len(bool_arrays[name]) + 31) // 32
            uname = name.upper()
            cname = safe_c_name(name)
            fh.write(f'extern const uint32_t tileprop_{cname}[{word_count}];\n')
            fh.write(f'#define TILEPROP_{uname}_EXISTS  1\n')
            fh.write(f'#define TILEPROP_{uname}_WORDS   {word_count}\n')
            fh.write(f'#define TILE_HAS_{uname}(tile_idx)  '
                     f'((tileprop_{cname}[(tile_idx) >> 5] >> ((tile_idx) & 0x1F)) & 1)\n\n')

        for name in sorted(int_arrays.keys()):
            tc = len(int_arrays[name])
            uname = name.upper()
            cname = safe_c_name(name)
            fh.write(f'extern const int32_t tileprop_{cname}[{tc}];\n')
            fh.write(f'#define TILEPROP_{uname}_EXISTS  1\n')
            fh.write(f'#define TILE_GET_{uname}(tile_idx)  (tileprop_{cname}[tile_idx])\n\n')

        for name in sorted(float_arrays.keys()):
            tc = len(float_arrays[name])
            uname = name.upper()
            cname = safe_c_name(name)
            fh.write(f'extern const float tileprop_{cname}[{tc}];\n')
            fh.write(f'#define TILEPROP_{uname}_EXISTS  1\n')
            fh.write(f'#define TILE_GET_{uname}(tile_idx)  (tileprop_{cname}[tile_idx])\n\n')

    if obj_groups:
        fh.write('/* === Object layer data === */\n')
        for otype, objs in obj_groups.items():
            ctype = safe_c_name(otype)
            utype = safe_c_name(otype).upper()
            fh.write(f'extern const map_obj_t map_obj_{ctype}[{len(objs)}];\n')
            fh.write(f'#define MAP_OBJ_{utype}_COUNT  {len(objs)}\n\n')

    fh.write(f'#endif /* {guard} */\n')


def write_source(fc, h_basename, map_w, map_h, tilesets, layers,
                 bool_arrays, int_arrays, float_arrays, obj_groups,
                 prop_firstgids):
    """Generate map_data.c."""
    fc.write('/* Auto-generated by tiled2c.py -- DO NOT EDIT */\n')
    fc.write(f'#include "{h_basename}"\n')
    fc.write('#include <string.h>\n\n')

    if layers:
        fc.write('/* === Layer data === */\n\n')
        for lyr in layers:
            fc.write(f'const int16_t map_layer_{lyr["cname"]}[{map_w * map_h}] = {{\n')
            for r in range(0, len(lyr["data"]), map_w):
                row = lyr["data"][r:r + map_w]
                fc.write('    ' + ', '.join(str(v) for v in row) + ',\n')
            fc.write('};\n\n')

    if bool_arrays or int_arrays or float_arrays:
        fc.write('/* === Tile properties === */\n\n')

        for name in sorted(bool_arrays.keys()):
            write_bool_bitmask(fc, safe_c_name(name), bool_arrays[name])

        for name in sorted(int_arrays.keys()):
            write_int_array(fc, safe_c_name(name), int_arrays[name])

        for name in sorted(float_arrays.keys()):
            write_float_array(fc, safe_c_name(name), float_arrays[name])

        fc.write('/* === Property lookup table === */\n')
        entries = []
        for name in sorted(bool_arrays.keys()):
            wc = (len(bool_arrays[name]) + 31) // 32
            fgid = prop_firstgids.get(name, 0)
            entries.append(f'    {{{fgid}, "{name}", TILEPROP_TYPE_BOOL, '
                           f'tileprop_{safe_c_name(name)}, {wc}}}')
        for name in sorted(int_arrays.keys()):
            tc = len(int_arrays[name])
            fgid = prop_firstgids.get(name, 0)
            entries.append(f'    {{{fgid}, "{name}", TILEPROP_TYPE_INT, '
                           f'tileprop_{safe_c_name(name)}, {tc}}}')
        for name in sorted(float_arrays.keys()):
            tc = len(float_arrays[name])
            fgid = prop_firstgids.get(name, 0)
            entries.append(f'    {{{fgid}, "{name}", TILEPROP_TYPE_FLOAT, '
                           f'tileprop_{safe_c_name(name)}, {tc}}}')

        fc.write(f'const tileprop_desc_t tileprop_table[{len(entries)}] = {{\n')
        if entries:
            fc.write(',\n'.join(entries))
            fc.write('\n')
        fc.write('};\n\n')
        fc.write(f'const int tileprop_table_size = {len(entries)};\n\n')

    if obj_groups:
        fc.write('/* === Object layer data === */\n\n')
        write_object_data(fc, obj_groups)


def parse_tileset(tsx_path, firstgid):
    """Parse .tsx file, return tileset info dict."""
    tree = ET.parse(tsx_path)
    root = tree.getroot()

    ts_name = root.attrib.get('name', 'unnamed')
    tilecount = int(root.attrib.get('tilecount', 0))
    columns = int(root.attrib.get('columns', 0))
    tilewidth = int(root.attrib.get('tilewidth', 0))
    tileheight = int(root.attrib.get('tileheight', 0))

    img = root.find('image')
    img_source = img.attrib.get('source', '') if img is not None else ''
    img_width = int(img.attrib.get('width', 0)) if img is not None else 0
    img_height = int(img.attrib.get('height', 0)) if img is not None else 0

    prop_types = discover_tile_properties(root)
    bool_arrs, int_arrs, float_arrs = build_tile_property_arrays(
        root, prop_types, tilecount)

    return {
        'name': ts_name,
        'firstgid': firstgid,
        'tilecount': tilecount,
        'columns': columns,
        'tilewidth': tilewidth,
        'tileheight': tileheight,
        'img_source': img_source,
        'img_width': img_width,
        'img_height': img_height,
        'bool_arrays': bool_arrs,
        'int_arrays': int_arrs,
        'float_arrays': float_arrs,
    }


def main():
    if len(sys.argv) < 4:
        print("Usage: python tiled2c.py <map.tmx> <map_data.c> <map_data.h>", file=sys.stderr)
        sys.exit(1)

    tmx_path = sys.argv[1]
    out_c = sys.argv[2]
    out_h = sys.argv[3]

    tmx_dir = os.path.dirname(os.path.abspath(tmx_path))

    tree = ET.parse(tmx_path)
    root = tree.getroot()

    map_w = int(root.attrib.get('width', 0))
    map_h = int(root.attrib.get('height', 0))
    tile_w = int(root.attrib.get('tilewidth', 0))
    tile_h = int(root.attrib.get('tileheight', 0))

    # Parse tilesets
    tilesets = []
    for ts_elem in root.findall('tileset'):
        firstgid = int(ts_elem.attrib.get('firstgid', 1)) - 1  # Tiled 1-based → 0-based
        source = ts_elem.attrib.get('source', '')
        if source:
            tsx_path = os.path.join(tmx_dir, source)
            if not os.path.exists(tsx_path):
                print(f"Error: tileset file not found: {tsx_path}", file=sys.stderr)
                sys.exit(1)
            ts_info = parse_tileset(tsx_path, firstgid)
        else:
            print("Warning: embedded tileset not fully supported, skipping properties", file=sys.stderr)
            ts_info = {
                'name': ts_elem.attrib.get('name', 'unnamed'),
                'firstgid': firstgid,
                'tilecount': int(ts_elem.attrib.get('tilecount', 0)),
                'columns': int(ts_elem.attrib.get('columns', 0)),
                'tilewidth': int(ts_elem.attrib.get('tilewidth', 0)),
                'tileheight': int(ts_elem.attrib.get('tileheight', 0)),
                'img_source': '', 'img_width': 0, 'img_height': 0,
                'bool_arrays': {}, 'int_arrays': {}, 'float_arrays': {},
            }
        tilesets.append(ts_info)

    # Merge properties with prefix for multi-tileset, track firstgid
    all_bool = {}
    all_int = {}
    all_float = {}
    prop_firstgids = {}
    for ts in tilesets:
        prefix = f'{safe_c_name(ts["name"])}_' if len(tilesets) > 1 else ''
        for name, arr in ts['bool_arrays'].items():
            merged = prefix + name
            all_bool[merged] = arr
            prop_firstgids[merged] = ts['firstgid']
        for name, arr in ts['int_arrays'].items():
            merged = prefix + name
            all_int[merged] = arr
            prop_firstgids[merged] = ts['firstgid']
        for name, arr in ts['float_arrays'].items():
            merged = prefix + name
            all_float[merged] = arr
            prop_firstgids[merged] = ts['firstgid']

    # Parse layers
    layers = []
    for layer_elem in root.findall('layer'):
        name = layer_elem.attrib.get('name', 'unnamed')
        data = parse_layer_data(layer_elem)
        if data is None:
            print(f"Warning: layer '{name}' skipped (non-CSV encoding)", file=sys.stderr)
            continue
        if len(data) != map_w * map_h:
            print(f"Warning: layer '{name}' data size mismatch "
                  f"({len(data)} != {map_w * map_h}), padding/truncating", file=sys.stderr)
            while len(data) < map_w * map_h:
                data.append(-1)
            data = data[:map_w * map_h]
        layers.append({'name': name, 'cname': safe_c_name(name), 'data': data})

    obj_groups = parse_objects(root)

    # Generate header
    h_basename = os.path.basename(out_h)
    with open(out_h, 'w', encoding='utf-8') as fh:
        write_header(fh, h_basename, map_w, map_h,
                     tile_w, tile_h, tilesets, layers,
                     all_bool, all_int, all_float, obj_groups)

    # Generate source
    with open(out_c, 'w', encoding='utf-8') as fc:
        write_source(fc, h_basename, map_w, map_h, tilesets, layers,
                     all_bool, all_int, all_float, obj_groups, prop_firstgids)

    print(f"Generated: {out_h}, {out_c}")
    print(f"  Map: {map_w}x{map_h}, tile: {tile_w}x{tile_h}")
    print(f"  Layers: {len(layers)}")
    print(f"  Tilesets: {len(tilesets)}")
    if all_bool:
        print(f"  Bool properties: {', '.join(sorted(all_bool.keys()))}")
    if all_int:
        print(f"  Int properties: {', '.join(sorted(all_int.keys()))}")
    if all_float:
        print(f"  Float properties: {', '.join(sorted(all_float.keys()))}")
    if obj_groups:
        print(f"  Object groups: {', '.join(f'{k}({len(v)})' for k, v in obj_groups.items())}")


if __name__ == '__main__':
    main()
