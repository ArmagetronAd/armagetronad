# Code review: PR #7 — Optional TRON Legacy graphics with a Classic selector

> Posted here because the sandbox GitHub proxy blocks mutating API calls (comments/reviews). Original intent: comment on https://github.com/ArmagetronAd/armagetronad/pull/7

**Verdict: Approve, with minor nits.** The PR adds an opt-in "TRON Legacy" (blacklight) graphics style alongside the existing Classic look, selectable from a persistent Display Settings menu. It is large (90 files, +8864/−526) but the additions are mostly new, self-contained files; changes to existing core files are surgical. I reviewed the full diff in a local clone (base `c2669d7`, head `74c6584`).

## What the PR does

- **Style selector**: New "Graphics Style" entry in the Display Settings menu (Classic / TRON Legacy), persisted via `GRAPHICS_STYLE` in `src/tron/gGraphicsStyle.{h,cpp}`. Live-switching at runtime; a Classic style is always available as fallback.
- **New visuals** (Legacy only): sculpted lightcycle model (binary mesh + procedural fallback), arena backdrop stadium, translucent light-ribbon walls with crisp edges, bloom post-processing, floor reflection shader, smoothed chase camera, per-player colors, warm-spectrum AI palette.
- **Quality settings**: MSAA (`ANTIALIAS_SAMPLES`, with fallback retry) and texture anisotropy (`TEXTURE_ANISOTROPY`).
- **Infrastructure**: config-load phases (`tCallbackConfigLoad`), two-phase alpha rendering (`eGameObject::RenderEarly`/`RendersAlpha`), engine↔game callback registration (`se_RegisterLegacyGraphicsFunc`, mirroring the existing moviepack pattern), doctest unit tests, CI workflow, docs.

## Strengths

1. **Licensing/attribution is handled properly.** The bike model is a CC BY 3.0 derivative; `models/lightcycle-LICENSE.txt` + `lightcycle-CREDITS.txt` ship alongside the binary and match the BlendSwap license (I diffed them — identical). The .blend source, the Blender exporter (`export_game.py`), and credits are included in `art/`. Code stays GPL-2.0-or-later. Dual licensing is documented in `models/AGENTS.md`.
2. **Asset loader is bounded and transactional** (`gLightcycleAsset.h`): magic check, per-part/total vertex caps, coordinate/normal-length sanity checks, exact EOF check. I parsed the shipped `lightcycle-authored.bin` manually — well-formed (LCMESH1, 4 groups, 51 744 triangles, 10 floats/vertex). The unit test verifies rejection of truncation, trailing bytes, corrupt magic, and NaN.
3. **Presentation/simulation separation is respected.** Camera smoothing (`eCameraMotion::Follow`) and the assembly animation (`gLightcycleMotion::State`) are presentation-only; collision geometry is untouched.
4. **Preset hygiene is verified.** `graphics_classic.cfg`/`graphics_legacy.cfg` contain appearance settings only — I confirmed no resolution/FPS/AA keys, and `gGraphicsStyle_test.cpp` explicitly asserts `MAX_FPS`/`WINDOW_W`/`WINDOW_H`/`ANTIALIAS_SAMPLES` survive a style switch.
5. **Engine/game boundary preserved.** The engine never links game globals; detection goes through the registered callback, same as the existing moviepack hook.
6. **Tests**: 7 new doctest files (~975 lines), including a fixture-based test for the style/persistence logic.
7. **CI**: `.github/workflows/graphics-build.yml` builds client_debug + server_debug on ubuntu-24.04, pins the checkout action by SHA, `persist-credentials: false`, `contents: read` only. Also fixes two real dedicated-build bugs (`AC_LANG_PUSH([C])` + `AC_SEARCH_LIBS([sinf],[m])` in `configure.ac`).

## Findings (nits)

1. **Missing help strings (minor)**: `BLOOM_HALO` and `BLOOM_VIGNETTE` are registered settings (`rBloom.cpp:61-62`) and persisted, but `bloom_halo_help`/`bloom_vignette_help` are absent from `language/english_base.txt` (only `bloom_help`, `bloom_intensity_help`, `bloom_threshold_help`, `bloom_radius_help` exist). Same for `TEXTURE_ANISOTROPY` — registered in `rTexture.cpp:52`, but no help string and no menu entry (it's also not exposed in any menu; `gMenus.cpp` has no bloom/aniso/antialias entries at all). `ANTIALIAS_SAMPLES` likewise has a conf item but no menu/help. Not blockers — settings still work via config files — but the new quality settings are undiscoverable in-game.
2. **Dev/capture scaffolding shipped in trunk (minor)**: `--hidden-window` and `--videoout-interval` CLI switches (`rSysdep.cpp`), `CAPTURE_MODEL_VIEW` (`gCycle.cpp:2098`), `CAPTURE_TRAIL_DIAGNOSTICS` (`gWall.cpp:940`), and ~10 `if (sr_hiddenWindow) std::cerr << ...` log lines across `rScreen.cpp`, `gCycle.cpp`, `gWall.cpp`, `gExplosion.cpp`, `gLightcycleModel.cpp`, and the two material shaders. Functional and guarded, but this is capture/diagnostic tooling — maintainers may want it stripped or behind a build flag before merge.
3. **Binary assets (~13 MB)**: 2 `.blend` files (~6.5 MB), `lightcycle-authored.bin` (6.2 MB), `lightcycle_suit_mask.png` (202 KB). Including sources + exporter mitigates this, but it's a notable repo-weight increase.
4. **Could not verify compilation locally.** The sandbox has no root and package installs are blocked (no SDL2/GLEW/boost/protobuf dev packages), so I could not build or run the unit tests. The provided CI workflow covers this, but I couldn't observe it running — **please rely on CI before merge**. The code reads cleanly, but there may be compile issues I can't catch by inspection.
5. **Watch items (no action needed)**:
   - `gExplosion`'s constructor dereferences `owner_` for the new impact snapshots — same pattern trunk already uses for `radius_`, so no regression.
   - `gCycle::RefreshVisuals` early-return `return body || customModel;` — if Legacy resources fail to load, `InitAfterCreation` raises `tERR_ERROR` and `Render` just skips drawing. Acceptable failure mode.
   - `uMenuItemSelection::LeftRight` fires `onselect` even when the value is unchanged, so `sg_GraphicsStyleSelected` calls `st_SaveConfig()` on every left/right press. `sg_SelectGraphicsStyle` is idempotent for the same style, so it's a redundant save, not a bug.
   - Wall display-list caching: I traced `gCycleWallRenderer::RenderPassWithState` → static previous-state invalidation via `rDisplayList::ClearAll()`, and the `prepared_` membership coordination between EdgePass/MembranePass. The inhibit/record semantics in `rDisplayList` check out; no stale-cache path found.
   - `gCycle::~gCycle` now unconditionally deletes `customTexture`/`wheelTex`/`bodyTex` — safe because `Commit` swaps pointers and the temporary `gCycleVisuals` deletes leftovers; textures self-register in `rITexture`'s static list.

## Area-by-area breakdown of the 90-file diff

| Area | Files | Assessment |
|---|---|---|
| Build/CI | `configure.ac`, `src/Makefile.am`, `Makefile.am`, `config/Makefile.am`, `.github/workflows/graphics-build.yml` | Correct; all new sources wired into the right libraries; presets installed; CI sane. Fixes two dedicated-build libm bugs. |
| Persistence/menus | `gGraphicsStyle.{h,cpp}`, `tConfiguration.{h,cpp}`, `gStuff.{h,cpp}`, `gMenus.cpp`, `english_base.txt`, `graphics_*.cfg` | Clean. Config-load phase ordering is well thought out (appearance prefs restore after resource defaults, before autoexec). Menu refresh + save on select. Missing help strings for 4 settings (see nit 1). |
| Rendering — cycle | `gCycle.{h,cpp}`, `gCycleVisuals.{h,cpp}`, `gLightcycleModel.{h,cpp}`, `gLightcycleMaterial.h`, `gLightcycleEffects.{h,cpp}`, `gLightcycleMotion.h`, `gLightcycleProfile.h`, `gLightColour.h`, `gExplosion.{h,cpp}`, `gSparks.cpp`, `gAIBase.cpp` | Largest area. Live style switching, two-phase alpha rendering, assembly animation, blacklight explosion flash. Procedural fallback geometry is a nice touch. Shaders are GLSL 120 with fixed-function fallback. |
| Rendering — walls/floor | `gWall.{h,cpp}`, `gLightwallMaterial.h`, `gLightwallAttachment.h`, `gFloor.cpp`, `gArenaMaterial.h`, `gArenaBackdrop.{h,cpp}`, `eDisplay.cpp` | Wall renderer split into EdgePass/MembranePass with shared membership; attachment curves for the moving cut segment; floor spill; rim backdrop culled to outer faces; Fresnel reflection shader gated on Legacy + FOG + GL 2.0. |
| Rendering — engine | `eGameObject.{h,cpp}`, `eCamera.cpp`, `eCameraMotion.h`, `ePlayer.{h,cpp}`, `eAdvWall.{h,cpp}`, `rScreen.{h,cpp}`, `rSysdep.cpp`, `rTexture.{h,cpp}`, `rFont.{h,cpp}`, `rBloom.{h,cpp}`, `gGame.cpp`, `gArmagetron.cpp`, `cockpit/cWidgetBase.cpp` | MSAA with fallback, anisotropy, hidden-window capture, bloom pass after viewports/before HUD with graceful-failure latch, font dark-backdrop for name labels, one cockpit loop fix. |
| Assets | `art/lightcycle-authored/**`, `models/lightcycle-authored.bin`, `models/lightcycle-{CREDITS,LICENSE}.txt`, `textures/lightcycle_suit_mask.png`, `resource/proto/Romil/blacklight.aacockpit.xml` | Licensing and attribution complete; exporter included; binary validated well-formed. |
| Tests | 7 new files in `src/test/` + `src/Makefile.am` wiring | Good coverage of the new logic; asset loader test loads the shipped binary. |
| Docs | 11 `AGENTS.md` files updated | Consistent with the per-directory doc convention. |

## Suggested pre-merge actions

1. Add `bloom_halo_help`, `bloom_vignette_help`, and help strings (and ideally menu entries) for `TEXTURE_ANISOTROPY` and `ANTIALIAS_SAMPLES` — or drop the settings to config-only and document that.
2. Decide whether the capture scaffolding (`--hidden-window`, `--videoout-interval`, `CAPTURE_MODEL_VIEW`, `CAPTURE_TRAIL_DIAGNOSTICS`, `sr_hiddenWindow` logging) belongs in trunk or should be stripped/flagged.
3. Confirm CI (client_debug + server_debug) is green, since I could not build locally.

Happy to help iterate on any of the nits if useful.
