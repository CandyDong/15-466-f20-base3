EXPORT_MESHES=export-meshes.py
EXPORT_SCENE=export-scene.py

DIST=../dist

all : \
	$(DIST)/scene.pnct \
	$(DIST)/scene.scene \


$(DIST)/scene.scene : scene.blend $(EXPORT_SCENE)
	blender --background --python $(EXPORT_SCENE) -- '$<':Collection '$@'

$(DIST)/scene.pnct : scene.blend $(EXPORT_MESHES)
	blender --background --python $(EXPORT_MESHES) -- '$<':Collection '$@'
