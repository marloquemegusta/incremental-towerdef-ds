FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    blender \
    python3-pil \
    python3-numpy \
    libgl1 \
    libglu1-mesa \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Fix NumPy 1.24+ deprecated np.bool alias in Blender 3.4 glTF importer
RUN find /usr/share/blender/scripts/addons/io_scene_gltf2 -type f -name "*.py" -exec sed -i 's/np\.bool/bool/g' {} +

WORKDIR /workspace
ENTRYPOINT ["blender"]
