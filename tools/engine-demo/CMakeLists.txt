# Small program exercising vs_engine and vs_editor (rendering, text, audio, UI, tile editor).
#   engine-demo [--smoke]           game-style demo scene
#   engine-demo --editor [--smoke]  demo map editor built on vs_editor
add_executable(engine-demo WIN32
  src/main.cpp
  src/demo_scene.cpp
  src/demo_editor.cpp
  res/app.rc)
target_link_libraries(engine-demo PRIVATE vs_engine vs_editor)
