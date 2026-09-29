#!/usr/bin/env python3
"""
render_spritesheet.py - Headless Blender Isometric Spritesheet Renderer
Generates multi-directional spritesheets (Starcraft / Factorio style) from 3D models.
"""

import sys
import os
import math
from pathlib import Path
from PIL import Image

# Fix numpy deprecations for older Blender glTF addons
try:
    import numpy as np
    if not hasattr(np, 'bool'):
        np.bool = bool
    if not hasattr(np, 'float'):
        np.float = float
    if not hasattr(np, 'int'):
        np.int = int
except Exception:
    pass

import bpy

def parse_args():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    cfg = {
        "model": "",
        "anim": "walk",
        "dirs": 8,
        "res": 64,
        "frames": 8,
        "engine": "CYCLES",
        "samples": 16,
        "out": "spritesheet.png",
        "temp_dir": "/tmp/sprite_frames"
    }
    
    i = 0
    while i < len(args):
        arg = args[i]
        val = args[i+1] if i + 1 < len(args) else ""
        if arg in ["--model", "-m"]:
            cfg["model"] = val
            i += 2
        elif arg in ["--anim", "-a"]:
            cfg["anim"] = val
            i += 2
        elif arg in ["--dirs", "-d"]:
            cfg["dirs"] = int(val)
            i += 2
        elif arg in ["--res", "-r"]:
            cfg["res"] = int(val)
            i += 2
        elif arg in ["--frames", "-f"]:
            cfg["frames"] = int(val)
            i += 2
        elif arg in ["--engine", "-e"]:
            cfg["engine"] = val
            i += 2
        elif arg in ["--out", "-o"]:
            cfg["out"] = val
            i += 2
        elif arg in ["--temp"]:
            cfg["temp_dir"] = val
            i += 2
        else:
            i += 1
    return cfg

def clear_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)

def import_asset(model_path):
    ext = Path(model_path).suffix.lower()
    if ext == ".fbx":
        bpy.ops.import_scene.fbx(filepath=model_path)
    elif ext in [".gltf", ".glb"]:
        bpy.ops.import_scene.gltf(filepath=model_path)
    elif ext == ".obj":
        bpy.ops.wm.obj_import(filepath=model_path)
    else:
        raise ValueError(f"Formato no soportado: {ext}")

def setup_rig_and_cam(res, center_z, max_dim, engine="CYCLES", samples=16):
    scene = bpy.context.scene
    scene.render.engine = engine
    if engine == "CYCLES":
        scene.cycles.device = 'CPU'
        scene.cycles.samples = samples
        scene.cycles.preview_samples = samples
        scene.cycles.use_denoising = False
        for vl in scene.view_layers:
            if hasattr(vl, 'cycles'):
                vl.cycles.use_denoising = False
    
    scene.render.film_transparent = True
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGBA'
    scene.render.resolution_x = res
    scene.render.resolution_y = res
    scene.render.resolution_percentage = 100

    # Look-at target empty at model center
    target = bpy.data.objects.new("CamTarget", None)
    target.location = (0.0, 0.0, center_z)
    scene.collection.objects.link(target)

    # Rig Empty for 360-degree rotation (independent of character animation tracks)
    rig = bpy.data.objects.new("CameraRig", None)
    rig.location = (0.0, 0.0, center_z)
    scene.collection.objects.link(rig)

    # Orthographic Camera
    cam_data = bpy.data.cameras.new("IsoCam")
    cam_data.type = 'ORTHO'
    cam_data.ortho_scale = max_dim * 1.4
    cam_obj = bpy.data.objects.new("IsoCam", cam_data)
    scene.collection.objects.link(cam_obj)
    scene.camera = cam_obj

    # 60 deg isometric pitch (StarCraft / 2:1 dimetric projection)
    pitch = math.radians(60.0)
    dist = max_dim * 4.0
    cam_obj.location = (0.0, -dist * math.cos(pitch), dist * math.sin(pitch) + center_z)
    cam_obj.parent = rig
    
    # Track to target
    tt = cam_obj.constraints.new(type='TRACK_TO')
    tt.target = target
    tt.track_axis = 'TRACK_NEGATIVE_Z'
    tt.up_axis = 'UP_Y'

    # Key Light (Sun attached to rig for consistent lighting or studio angle)
    sun_data = bpy.data.lights.new("Sun", type='SUN')
    sun_data.energy = 5.0
    sun_obj = bpy.data.objects.new("Sun", sun_data)
    sun_obj.location = (dist, -dist, dist + center_z)
    sun_obj.parent = rig
    tt_sun = sun_obj.constraints.new(type='TRACK_TO')
    tt_sun.target = target
    tt_sun.track_axis = 'TRACK_NEGATIVE_Z'
    tt_sun.up_axis = 'UP_Y'
    scene.collection.objects.link(sun_obj)

    # Fill Light
    fill_data = bpy.data.lights.new("FillSun", type='SUN')
    fill_data.energy = 2.0
    fill_obj = bpy.data.objects.new("FillSun", fill_data)
    fill_obj.location = (-dist, -dist, dist * 0.5 + center_z)
    fill_obj.parent = rig
    tt_fill = fill_obj.constraints.new(type='TRACK_TO')
    tt_fill.target = target
    tt_fill.track_axis = 'TRACK_NEGATIVE_Z'
    tt_fill.up_axis = 'UP_Y'
    scene.collection.objects.link(fill_obj)

    return scene, rig

def apply_actions_to_objects(anim_query):
    actions = list(bpy.data.actions)
    print(f"[BLENDER] Acciones disponibles ({len(actions)}): {[a.name for a in actions]}")
    
    q = anim_query.lower()
    matching_actions = [a for a in actions if q in a.name.lower()]
    if not matching_actions:
        matching_actions = actions[:1]
        if matching_actions:
            print(f"[BLENDER] No se encontro accion '{anim_query}', usando '{matching_actions[0].name}'")
        else:
            print("[BLENDER] No se encontraron animaciones en el modelo.")

    main_action = None
    for obj in bpy.data.objects:
        for act in matching_actions:
            if obj.name.lower() in act.name.lower() or (obj.type == 'ARMATURE' and 'armature' in act.name.lower()):
                if not obj.animation_data:
                    obj.animation_data_create()
                obj.animation_data.action = act
                print(f"[BLENDER] Asignada accion '{act.name}' a objeto '{obj.name}'")
                if not main_action or obj.type == 'ARMATURE':
                    main_action = act

    if not main_action and matching_actions:
        main_action = matching_actions[0]
        armatures = [o for o in bpy.data.objects if o.type == 'ARMATURE']
        if armatures:
            arm = armatures[0]
            if not arm.animation_data:
                arm.animation_data_create()
            arm.animation_data.action = main_action

    return main_action

def compute_model_bounds():
    mesh_objs = [o for o in bpy.data.objects if o.type == 'MESH']
    if not mesh_objs:
        return 0.5, 2.0
    
    all_z = []
    all_dims = []
    for o in mesh_objs:
        bbox_corners = [o.matrix_world @ mathutils.Vector(corner) for corner in o.bound_box] if hasattr(o, 'matrix_world') else []
        if not bbox_corners:
            bbox_corners = [mathutils.Vector(corner) + o.location for corner in o.bound_box]
        for v in bbox_corners:
            all_z.append(v.z)
        all_dims.extend([o.dimensions.x, o.dimensions.y, o.dimensions.z])
        
    min_z = min(all_z) if all_z else 0.0
    max_z = max(all_z) if all_z else 2.0
    center_z = (min_z + max_z) * 0.5
    max_dim = max(all_dims) if all_dims else (max_z - min_z)
    max_dim = max(1.0, max_dim)
    return center_z, max_dim

def render_pipeline(cfg):
    print(f"[BLENDER] Iniciando pipeline para: {cfg['model']}")
    clear_scene()
    
    if not os.path.exists(cfg["model"]):
        raise FileNotFoundError(f"Modelo no encontrado: {cfg['model']}")
    
    import_asset(cfg["model"])
    
    global mathutils
    import mathutils
    
    center_z, max_dim = compute_model_bounds()
    print(f"[BLENDER] Dimensiones: center_z={center_z:.2f}, max_dim={max_dim:.2f}")
    
    scene, rig = setup_rig_and_cam(cfg["res"], center_z, max_dim, cfg["engine"], cfg["samples"])
    main_action = apply_actions_to_objects(cfg["anim"])
    
    if main_action:
        start_f = int(main_action.frame_range[0])
        end_f = int(main_action.frame_range[1])
    else:
        start_f = scene.frame_start
        end_f = scene.frame_end

    total_anim_frames = max(1, end_f - start_f)
    num_output_frames = cfg["frames"]
    frame_indices = [int(start_f + (i * total_anim_frames) / num_output_frames) for i in range(num_output_frames)]
    
    num_dirs = cfg["dirs"]
    # StarCraft order:
    # 0 = South (Front)
    # 1 = South-West
    # 2 = West
    # 3 = North-West
    # 4 = North (Back)
    # 5 = North-East
    # 6 = East
    # 7 = South-East
    angle_step = 360.0 / num_dirs
    
    os.makedirs(cfg["temp_dir"], exist_ok=True)
    rendered_matrix = []
    
    print(f"[BLENDER] Renderizando {num_dirs} direcciones x {num_output_frames} frames ({num_dirs * num_output_frames} renders)...")
    
    for d in range(num_dirs):
        # Rotate camera rig around target: negative angle rotates camera clockwise = model appears turning counter-clockwise
        rig.rotation_euler.z = math.radians(-d * angle_step)
        bpy.context.view_layer.update()
        
        dir_frames = []
        for f_idx, f_num in enumerate(frame_indices):
            scene.frame_set(f_num)
            out_file = os.path.join(cfg["temp_dir"], f"d{d:02d}_f{f_idx:03d}.png")
            scene.render.filepath = out_file
            bpy.ops.render.render(write_still=True)
            dir_frames.append(out_file)
        rendered_matrix.append(dir_frames)

    # Stitch into standard Spritesheet Grid (Width: Frames, Height: Directions)
    sheet_w = num_output_frames * cfg["res"]
    sheet_h = num_dirs * cfg["res"]
    sheet = Image.new("RGBA", (sheet_w, sheet_h), (0, 0, 0, 0))

    for d_idx, row in enumerate(rendered_matrix):
        for f_idx, file_path in enumerate(row):
            img = Image.open(file_path)
            sheet.paste(img, (f_idx * cfg["res"], d_idx * cfg["res"]))
            img.close()
            try:
                os.remove(file_path)
            except Exception:
                pass

    try:
        os.rmdir(cfg["temp_dir"])
    except Exception:
        pass

    out_path = Path(cfg["out"])
    out_path.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(str(out_path), format="PNG", optimize=False)
    print(f"[BLENDER SUCCESS] Spritesheet generado: {out_path} ({sheet_w}x{sheet_h} px)")

if __name__ == "__main__":
    config = parse_args()
    render_pipeline(config)
