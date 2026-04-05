# Test Cases — 700105 Simulation and Concurrency Final Lab

Each test case lists: **Preconditions → Steps → Expected result → Common failures**.

All tests run against `x64/Release/simulation-engine.exe` (or Debug for validation-layer output).

---

## TC-001: Scene Reset (Regression — Vulkan Safety)

**Preconditions:** Engine running, any scene loaded. Ideally run with Vulkan validation layers enabled (Debug build) to check for errors.

**Steps:**
1. Launch the engine. Wait for the default scene to fully load.
2. Open the **Scenario** menu in the top menu bar.
3. Click **"Reset Current Scenario"**.
4. Watch the log output at the bottom of the window.

**Expected:**
- Scene reloads within ~1 second.
- No Vulkan validation errors in the log (`VK_ERROR_DEVICE_LOST`, `vkCmdBindPipeline invalid`, etc. must be absent).
- Frame rate returns to normal after reload.

**Before fix (historic):** Console flooded with `vkCmdBindPipeline invalid because pipeline was destroyed`.

**Common failures:**
- Validation errors after reset → `changeScenario()` was called directly from ImGui; ensure `requestScenarioChange()` is used instead (see `source/services/DebugOverlay.cpp`).

---

## TC-002: Cloth Wind

**Preconditions:** Load `config/flatbufferConfig/cloth_test.bin` via **Scene** menu (or it loads as default).

**Steps:**
1. Wait 1–2 seconds for the cloth to settle under gravity.
2. Open **Cloth** menu in the top menu bar.
3. Drag the **"Wind X"** slider to **+5.0**.
4. Observe the cloth.

**Expected:**
- Cloth billows to the left (negative X direction) — hanging particles visibly displace horizontally while pinned top edge stays fixed.

5. Drag **"Wind Z"** to **+5.0** simultaneously.
6. Cloth should billow diagonally (left and into the screen).
7. Set both sliders back to **0.0**.

**Expected after reset:**
- Cloth settles back to purely vertical hang under gravity within a few seconds.

**Common failures:**
- No movement → cloth particles may all be pinned; check `pin_top_edge` in cloth_test.json (should be `true`, pinning only the top row).
- Cloth explodes → spring constants too low; press Ctrl+Z won't help here; reload the scene.

---

## TC-003: Cloth Tearing

**Preconditions:** Load `config/flatbufferConfig/cloth_test.bin`. Wait for the falling sphere to approach the cloth.

**Steps:**
1. Open **Cloth** menu.
2. Drag **"Tear Threshold"** slider to **1.3** (springs tear when stretched to 130% of rest length).
3. The sphere in the scene hits the cloth; watch for tearing near the impact point.
4. Torn sections should fall freely under gravity (they are no longer constrained by springs).

5. Lower **"Tear Threshold"** to **1.1** — cloth tears almost immediately when the scene loads, even from gravity.
6. Click **"Reset Springs"** button.

**Expected after Reset Springs:**
- All `ClothSpring::active` flags are set to `true`.
- Cloth hangs whole again from the pinned top edge.

**Common failures:**
- No tearing at 1.3 → threshold may be 3.0 (default); confirm slider moved.
- Reset Springs doesn't work → verify the button calls `for (auto& s : cc.springs) s.active = true;` in the ImGui handler.

---

## TC-004: Cloth Burning

**Preconditions:** Load `config/flatbufferConfig/cloth_test.bin`. Wait for cloth to settle (2–3 seconds after load).

**Steps:**
1. Open **Cloth** menu.
2. Tick the **"Enable Burn"** checkbox. *(This activates the burn heat pass each physics tick.)*
3. Click **"Place Burn at Centre"** button. *(This computes the average particle position and sets burnCenter there.)*
4. Check the status line below the button: **"Burn center: X.X, Y.Y, Z.Z"** — the Y value should now be a plausible world-space value (e.g. around 1.0–3.0), NOT **−1000.0** (the default inactive value).
5. Observe the cloth over the next 1–2 seconds.

**Expected:**
- Particles within `burnRadius` (default 0.8) of the burn center begin changing colour: cloth colour → **orange/red** (heat rising from 0→1).
- After ~0.7 s at default burn rate 1.5: central particles turn **near-black** and detach from the cloth, falling freely under gravity.
- The visible "burn hole" spreads outward as neighbouring particles fall into the burn zone.

6. Click **"Reset Springs"** to restore the cloth. *(Note: burned particles remain burned; Reset Springs only re-enables torn springs.)*

**Common failures:**
- Nothing happens → "Enable Burn" not checked, OR burn center is still at Y=−1000 (button not clicked). Check the status line coordinate.
- Cloth changes position but no colour → vertex colour update loop might not be active; ensure `FlatBuffersScenario::OnUpdate()` heat-blending code is present.
- Burn spreads instantly → `burnRate` slider is very high; lower it.

---

## TC-005: Flocking — Spatial Mode Comparison

**Preconditions:** Load `config/flatbufferConfig/flock_test.bin`.

**Steps:**
1. Open **Flocking** menu.
2. Note the **"Neighbour checks: N"** and **"Update time: X ms"** counters with **Brute Force** selected.
   - For 60 agents: expect ~3600 checks/frame (N²).
3. Switch to **"Uniform Grid"** in the Spatial Mode combo.
   - Expected: neighbour check count drops significantly (typically 100–500 checks for 60 agents with default radii).
4. Switch to **"Octree"**.
   - Expected: similar or slightly different reduction to Uniform Grid.

5. Test steering weights:
   - Drag **"W Separation"** to **0.0** → agents should cluster together (no personal space; they converge).
   - Drag **"W Cohesion"** to **0.0** → agents drift apart with no cohesion force.
   - Restore both to original values (W Sep = 2.0, W Coh = 1.0) → flock reforms.

**Expected:**
- Brute Force: O(N²) checks.
- Uniform Grid and Octree: substantially fewer checks, lower update time.
- Steering weight changes produce visible behaviour changes in real-time.

**Common failures:**
- All agents fly off screen → initial spawn radius too large or `max_force` too high; reload.
- Octree performance worse than Brute Force → expected for very uniform/sparse distributions; acceptable.

---

## TC-006: Spawner with Single-Instance Ownership

**Preconditions:** Load `config/flatbufferConfig/showcases/spawner_demo.bin`. Open the **Network** menu and set **Local Peer ID = 1**.

**Steps:**
1. Wait for **t = 2.0 s** after scene load (watch log or count).
2. **Spawner 1** (owner ONE = Peer 1) fires: 8 rubber spheres should appear scattered above the floor and bounce.
3. Observe **Spawner 2** (owner TWO = Peer 2): since local peer is 1, this spawner will NOT fire locally. It requires a connected Peer 2 instance.
4. Observe **Spawner 3** (SEQUENTIAL): the first entity spawns assigned to Peer 1 (red), second would be Peer 2, etc. Since only Peer 1 is connected, only the first spawn in each cycle fires locally.

**Expected (single instance, Peer 1):**
- Spawner 1 fires at t=2s: 8 spheres drop and bounce.
- Spawner 2 does not fire (owned by Peer 2 who is not connected).
- Spawner 3 fires one entity per cycle (Peer 1's slot), with cycling IDs for future peers.

**Two-instance test:**
1. Launch Instance A: Peer ID = 1. Launch Instance B: Peer ID = 2.
2. Connect: Instance A → enter Instance B's IP + port, click Connect.
3. On Instance A: Spawner 1 fires at t=2s. Instance B should see the spheres appear via `SpawnObject` network packet.
4. On Instance B: Spawner 2 fires every 1.5s. Instance A should see the boxes appear.

**Common failures:**
- Spheres appear but fly through the floor → material interaction not loaded; check `materials` and `interactions` sections in spawner_demo.json.
- Network peer sees nothing → `BroadcastSpawnObject()` may not be called; verify SpawnerSystem fires only for local owner and broadcasts.

---

## TC-007: Animated Objects + Collision Momentum Transfer

**Preconditions:** Load `config/flatbufferConfig/showcases/animation_demo.bin`.

**Steps:**
1. Identify the three platforms:
   - **Platform A** (left back): moves left→right, then **stops**. Does NOT return.
   - **Platform B** (centre): smooth loop left↔right↔up with SMOOTHSTEP easing — visible deceleration at turning points.
   - **Platform C** (right): constant-speed bounce up↔down with REVERSE path mode.

2. Confirm Platform A stops after 4 seconds and stays at its end position.
3. Confirm Platform B decelerates noticeably near each waypoint (SMOOTHSTEP makes the motion ease in/out). Compare against Platform C which moves at constant speed.
4. A rubber sphere (SphereB, green) is dropped above Platform B. When the platform is moving upward and hits the sphere, the sphere should receive an upward velocity boost (momentum transfer from the animated platform's velocity).

**Expected:**
- Platform A: moves once, stops.
- Platform B: smooth deceleration visible at each waypoint. LOOP — returns to start indefinitely.
- Platform C: constant-speed ping-pong (REVERSE mode).
- SphereB receives visible upward impulse when struck by the rising platform.

**Common failures:**
- Spheres fall through platforms → AnimatedObjectComponent entities need a collider (cuboid); ensure `adaptShape()` runs for animated objects and adds a `CuboidCollider`.
- Platform B doesn't decelerate visibly → confirm `easing: "SMOOTHSTEP"` in the JSON.

---

## TC-008: Dead Reckoning Smoothness

**Preconditions:** Two instances running locally (or across LAN). Both connected (Peer 1 and Peer 2).

**Steps:**
1. Load `config/flatbufferConfig/test_fb_scene.bin` on both instances.
2. On Instance B, open **Simulation** menu → lower **Physics Hz** to **10** (simulates poor update rate).
3. Observe the remote sphere (owned by Peer 1) on Instance B.

**Expected:**
- Even at 10 Hz physics (100ms between state packets), the remote sphere on Instance B moves **smoothly** between updates using dead reckoning (predicted position = authPosition + authVelocity × timeSincePacket).
- When a new packet arrives, the position snaps slightly but then smoothly blends (120ms blend window) rather than teleporting.
- No visible "jumping" or freezing of the remote sphere.

4. Restore Physics Hz to 120 on both.

**Expected after restore:**
- Remote entity movement is tight and responsive with low latency.
- **Network** menu → **"Tracked remote entities"** count shows 1 (the peer's sphere).

**Common failures:**
- Remote entity teleports → blendTimer not being decremented; check `UpdateRemoteEntities()`.
- Remote entity freezes after 2s → 2-second timeout guard triggered; verify packets are being sent.

---

## TC-009: Process Affinity Verification

**Preconditions:** Engine running (Debug or Release). Log window visible.

**Steps:**
1. Launch `simulation-engine.exe`.
2. Check the startup log for thread CPU assignment messages:
   - `"MainThread on CPU 0"` (Core 1, affinity mask `0x01`)
   - `"PhysicsThread started, CPU 3"` (Core 4, affinity mask `0x08`)  
   - `"NetworkingThread started, CPU 1"` or `"CPU 2"` (Cores 2–3, affinity mask `0x06`)
3. Open **Windows Task Manager → Details** tab.
4. Right-click `simulation-engine.exe` → **Set Affinity**.
5. Observe that different threads are pinned to different cores.

**Expected:**
- Log confirms three distinct CPU assignments.
- Task Manager shows process using multiple cores.

**Common failures:**
- All threads on CPU 0 → `SetThreadAffinityMask()` failed or wasn't called; check return value in `EngineOrchestrator`.
- CPU numbers are off-by-one → Windows uses 0-indexed CPU numbers in `GetCurrentProcessorNumber()`; affinity mask bit 0 = CPU 0 = "Core 1" in Task Manager's 1-indexed display.

---

## TC-010: Global Scene Switching (Network Sync)

**Preconditions:** Two instances connected (Peer 1 and Peer 2). Both on the same LAN or localhost.

**Steps:**
1. Both instances start on the default scene.
2. On **Instance A**: open **Scene** menu → select **cloth_test.bin**.

**Expected:**
- Instance A switches to cloth_test immediately.
- Instance B also switches to cloth_test within ~300ms (scene change is broadcast 3× for UDP reliability).

3. On **Instance B**: open **Scene** menu → select **flock_test.bin**.

**Expected:**
- Both instances switch to flock_test.

4. On **Instance A**: open **Network** menu → confirm toggle "Show Owner Colors" does NOT change on Instance B (this is local-only UI, not broadcast).

**Common failures:**
- Instance B doesn't switch scenes → `BroadcastSceneChange()` not called, or `PollPendingSceneChange()` not called each frame in `EngineOrchestrator::drawFrame()`.
- Both instances switch but cloth sim crashes → `ClearRemoteStates()` not called in `FlatBuffersScenario::OnLoad()`; stale entity IDs collide with new scene entities.
- Scene path not found on Instance B → both instances must have the same relative path to `config/flatbufferConfig/`. Ensure you're running from the project root directory.
