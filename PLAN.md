# chad-engine — Teaching Plan

User reimplements engine from scratch. Reference: `~/Repos/quakepg`. Claude tutors — no full solutions, hints only. 2+ failed attempts → minimal nudge.

---

## Rules
- Claude NEVES LEAVES CAVEMAN MODE
- User writes all code
- Claude reviews, points errors, asks questions
- After each module: build must compile clean (`-Wall -Wextra`)
- User pastes code here; Claude reviews before next step

---

## Roadmap

### Phase 1 — Foundation

| Step | File | What to implement | Status |
|------|------|-------------------|--------|
| 1.1 | `engine/include/core/types.h` | Type aliases: u8–u64, i8–i64, f32, f64, usize | ⬜ |
| 1.2 | `engine/include/core/math.h` | Vec2, Vec3, Vec4, Mat4 structs + free functions | ⬜ |
| 1.3 | `engine/include/core/log.h` | LOG_INFO / LOG_WARN / LOG_ERROR / LOG_FATAL macros | ⬜ |
| 1.4 | `engine/include/core/assert.h` | CHAD_ASSERT, CHAD_ASSERT_MSG | ⬜ |

### Phase 2 — Platform Layer

| Step | File | What to implement | Status |
|------|------|-------------------|--------|
| 2.1 | `engine/include/platform/window.h` + `.cpp` | GLFW window + OpenGL 3.3 core context | ⬜ |
| 2.2 | `engine/include/platform/input.h` + `.cpp` | Keyboard state (down/pressed/released) + mouse delta | ⬜ |
| 2.3 | `engine/include/platform/timer.h` + `.cpp` | Frame delta time measurement | ⬜ |

**Milestone:** Open window, poll input, print delta time. Build passes.

### Phase 3 — Renderer Core

| Step | File | What to implement | Status |
|------|------|-------------------|--------|
| 3.1 | `engine/include/renderer/shader.h` + `.cpp` | Compile vert/frag, link, set uniforms | ⬜ |
| 3.2 | `engine/include/renderer/mesh.h` + `.cpp` | VAO/VBO/EBO from Vertex array | ⬜ |
| 3.3 | `engine/include/renderer/texture.h` + `.cpp` | stb_image load, GL_NEAREST, fallback white | ⬜ |
| 3.4 | `engine/include/renderer/material.h` + `.cpp` | Shader + texture + tint color combo | ⬜ |
| 3.5 | `engine/include/renderer/camera.h` + `.cpp` | FPS camera: yaw/pitch, view/proj matrices | ⬜ |
| 3.6 | `engine/include/renderer/renderer.h` + `.cpp` | FBO at internal res, nearest-neighbor upscale | ⬜ |

**Milestone:** Render colored triangle. Then textured quad. Build passes.

### Phase 4 — PSX Shaders

| Step | File | What to implement | Status |
|------|------|-------------------|--------|
| 4.1 | `assets/shaders/basic.vert/frag` | Pass-through: position + texcoord + color | ⬜ |
| 4.2 | `assets/shaders/psx.vert` | Vertex snapping (fixed-point GTE simulation) | ⬜ |
| 4.3 | `assets/shaders/psx.frag` | Dithering (Bayer 4×4) + fog + color depth reduction | ⬜ |

**Milestone:** Scene renders with PSX aesthetic.

### Phase 5 — 3D Models & Assets

| Step | File | What to implement | Status |
|------|------|-------------------|--------|
| 5.1 | `engine/include/renderer/model.h` + `.cpp` | Assimp loader: extract meshes + embedded textures | ⬜ |

**Milestone:** Load a GLB model and render it in scene.

### Phase 6 — Physics

| Step | File | What to implement | Status |
|------|------|-------------------|--------|
| 6.1 | `engine/include/physics/collision.h` + `.cpp` | AABB intersection + per-axis slide resolution | ⬜ |

**Milestone:** Player walks through dungeon without clipping through walls.

### Phase 7 — Dungeon

| Step | File | What to implement | Status |
|------|------|-------------------|--------|
| 7.1 | `game/include/dungeon/dungeon_map.h` + `.cpp` | BSP procedural dungeon → mesh + AABB colliders | ⬜ |

**chad improvement over quakepg:** BSP instead of ASCII map.

**Milestone:** Procedural dungeon renders. Player navigates it.

### Phase 8 — Weapon & View Model

| Step | File | What to implement | Status |
|------|------|-------------------|--------|
| 8.1 | `game/include/weapon.h` + `.cpp` | View-space weapon render, bob/sway, attack anim | ⬜ |

**chad improvements:** Generic animation system + Bloodthief hand feel (position, sway, inertia).

**Milestone:** Weapon visible in hand, bobs on walk, swings on attack.

### Phase 9 — Debug UI

| Step | File | What to implement | Status |
|------|------|-------------------|--------|
| 9.1 | `engine/include/core/console.h` + `.cpp` | ImGui console + command registration | ⬜ |
| 9.2 | `engine/include/core/debug_ui.h` + `.cpp` | PSX settings panel (res, fog, dither, snap) | ⬜ |

**Milestone:** Press `` ` `` to open console. Tweak PSX params live.

### Phase 10 — Roguelike Loop

| Step | File | What to implement | Status |
|------|------|-------------------|--------|
| 10.1 | TBD | Permadeath, items, floor progression | ⬜ |

**chad improvement:** Real roguelike loop vs no loop in quakepg.

---

## Improvements Over quakepg (Tracking)

| # | Improvement | Phase |
|---|-------------|-------|
| 1 | Smart pointers (`unique_ptr`, `shared_ptr`) | All phases |
| 2 | BSP procedural dungeon | 7 |
| 3 | Generic animation system | 8 |
| 4 | Bloodthief view model feel | 8 |
| 5 | Real roguelike loop | 10 |
| 6 | Per-vertex Gouraud lighting | 4 / 6 |

---

## Quality Gates (Every Phase)

- `cmake --build build -j$(nproc)` — zero errors, zero warnings
- No clang-tidy warnings
- ASan/UBSan clean (`-fsanitize=address,undefined`)
