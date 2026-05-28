-- =============================
-- 🌐 LOAD RAYFIELD
-- =============================
local Rayfield = loadstring(game:HttpGet('https://sirius.menu/rayfield'))()
local Window = Rayfield:CreateWindow({
    Name = "fake dehancements",
    Icon = 0,
    LoadingTitle = "Loading...",
    LoadingSubtitle = "sucking dick",
    ShowText = "Unhide",
    Theme = "Default",
    ToggleUIKeybind = "K",
    DisableRayfieldPrompts = true,
    DisableBuildWarnings = true,
    ConfigurationSaving = {
        Enabled = false,
        FolderName = nil,
        FileName = "combat_hub"
    },
    Discord = { Enabled = false },
    KeySystem = false
})

local Tab = Window:CreateTab("Combat", "skull")
Tab:CreateDivider()

-- =============================
-- 🧠 SERVICES
-- =============================
local Players = game:GetService("Players")
local RunService = game:GetService("RunService")
local Workspace = game:GetService("Workspace")
local UserInputService = game:GetService("UserInputService")

local LocalPlayer = Players.LocalPlayer

-- =============================
-- 📦 TELEPORT STATE
-- =============================
local distanceHistory = {}
local optimalDistance = 15
local enemyHealthCache = {}

local useManualDistance = false
local manualDistanceOverride = 15
local manualOrbitRate = 0.1
local orbitRate = 0.1

local chaseModeEnabled = false
local autoTeleportEnabled = false
local voidSpamEnabled = false
local antiKiciaEnabled = false
local orbitFarEnabled = false

local autoTeleportCoroutine = nil
local voidSpamCoroutine = nil
local antiKiciaConnection = nil
local orbitFarConnection = nil

local antiKiciaKey = Enum.KeyCode.P
local orbitFarKey = Enum.KeyCode.O

-- =============================
-- 🧠 UTILITIES
-- =============================
local function getRoot(char)
    return char and char:FindFirstChild("HumanoidRootPart")
end

local function getClosestOpponentHRP()
    local closestHRP = nil
    local minDist = math.huge
    local myRoot = getRoot(LocalPlayer.Character)
    if not myRoot then return nil end

    for _, plr in ipairs(Players:GetPlayers()) do
        if plr == LocalPlayer or not plr.Character then continue end
        local hrp = getRoot(plr.Character)
        if hrp then
            local dist = (myRoot.Position - hrp.Position).Magnitude
            if dist < minDist then
                minDist = dist
                closestHRP = hrp
            end
        end
    end
    return closestHRP
end

local function getHorizontalDistance(pos1, pos2)
    return math.sqrt((pos1.X - pos2.X)^2 + (pos1.Z - pos2.Z)^2)
end

local function safeTeleport(position)
    local root = getRoot(LocalPlayer.Character)
    if not root then return end
    root.CFrame = CFrame.new(position.X, position.Y, position.Z)
    local humanoid = LocalPlayer.Character and LocalPlayer.Character:FindFirstChildOfClass("Humanoid")
    if humanoid and Workspace.CurrentCamera then
        Workspace.CurrentCamera.CameraSubject = humanoid
    end
end

-- =============================
-- 📊 TARGETSTRAFE LEARNING
-- =============================
Players.PlayerAdded:Connect(function(plr)
    if plr == LocalPlayer then return end
    local char = plr.Character or plr.CharacterAdded:Wait()
    local humanoid = char:WaitForChild("Humanoid")
    enemyHealthCache[plr] = humanoid.Health

    humanoid:GetPropertyChangedSignal("Health"):Connect(function()
        local newHealth = humanoid.Health
        local oldHealth = enemyHealthCache[plr] or newHealth
        if newHealth < oldHealth and newHealth > 0 then
            local myRoot = getRoot(LocalPlayer.Character)
            local theirRoot = getRoot(char)
            if myRoot and theirRoot then
                local dist = math.floor(getHorizontalDistance(myRoot.Position, theirRoot.Position))
                if dist > 0 then
                    distanceHistory[dist] = (distanceHistory[dist] or 0) + 1
                    local bestDist, bestCount = 0, 0
                    for d, c in pairs(distanceHistory) do
                        if c > bestCount then
                            bestCount = c
                            bestDist = d
                        end
                    end
                    optimalDistance = math.max(bestDist, 5)
                end
            end
        end
        enemyHealthCache[plr] = newHealth
    end)
end)

-- =============================
-- 🌀 AUTO STRAFE (Close Orbit)
-- =============================
local function autoTeleportLoop()
    while autoTeleportEnabled do
        local myChar = LocalPlayer.Character
        local myRoot = getRoot(myChar)
        local humanoid = myChar and myChar:FindFirstChild("Humanoid")
        if not myRoot or not humanoid or humanoid.Health <= 0 then
            task.wait(1)
            continue
        end

        local closestEnemy = nil
        local minDist = math.huge
        for _, plr in ipairs(Players:GetPlayers()) do
            if plr ~= LocalPlayer and plr.Character then
                local theirRoot = getRoot(plr.Character)
                if theirRoot then
                    local d = getHorizontalDistance(myRoot.Position, theirRoot.Position)
                    if d < minDist then
                        minDist = d
                        closestEnemy = theirRoot
                    end
                end
            end
        end

        if not closestEnemy then
            task.wait(orbitRate)
            continue
        end

        local currentDistance = useManualDistance and manualDistanceOverride or optimalDistance
        local dir = (myRoot.Position - closestEnemy.Position)
        dir = Vector3.new(dir.X, 0, dir.Z).Unit
        local targetPos = closestEnemy.Position + dir * currentDistance
        safeTeleport(Vector3.new(targetPos.X, myRoot.Position.Y, targetPos.Z))

        task.wait(orbitRate)
    end
    autoTeleportCoroutine = nil
end

local function toggleAutoTeleport(state)
    autoTeleportEnabled = state
    if autoTeleportEnabled and not autoTeleportCoroutine then
        autoTeleportCoroutine = task.spawn(autoTeleportLoop)
    elseif not autoTeleportEnabled and autoTeleportCoroutine then
        task.cancel(autoTeleportCoroutine)
        autoTeleportCoroutine = nil
    end
end

-- =============================
-- 💥 VOIDSPAM
-- =============================
local function chaoticVoidTeleport()
    local root = getRoot(LocalPlayer.Character)
    if not root then return end
    local x = math.random(-250, 250)
    local y = -8
    local z = math.random(-3500, 3500)
    root.CFrame = CFrame.new(x, y, z)
end

local function chaseVoidSpamLoop()
    while voidSpamEnabled and chaseModeEnabled do
        local myChar = LocalPlayer.Character
        local humanoid = myChar and myChar:FindFirstChild("Humanoid")
        local myRoot = getRoot(myChar)
        if not humanoid or humanoid.Health <= 0 or not myRoot then break end

        local targetEnemy = nil
        local minDist = math.huge
        for _, plr in ipairs(Players:GetPlayers()) do
            if plr ~= LocalPlayer and plr.Character then
                local theirRoot = getRoot(plr.Character)
                if theirRoot then
                    local d = (myRoot.Position - theirRoot.Position).Magnitude
                    if d < minDist then
                        minDist = d
                        targetEnemy = theirRoot
                    end
                end
            end
        end

        if targetEnemy then
            local angle = math.random() * math.pi * 2
            local radius = useManualDistance and manualDistanceOverride or 10
            local offsetX = math.cos(angle) * radius
            local offsetZ = math.sin(angle) * radius
            local y = math.random(-80, -10)
            safeTeleport(Vector3.new(
                targetEnemy.Position.X + offsetX,
                y,
                targetEnemy.Position.Z + offsetZ
            ))
        else
            chaoticVoidTeleport()
        end

        task.wait(manualOrbitRate)
        if math.random() < 0.15 then
            task.wait(0.15 + math.random() * 0.2)
        end
    end
    voidSpamCoroutine = nil
end

local function normalVoidSpamLoop()
    while voidSpamEnabled and not chaseModeEnabled do
        local humanoid = LocalPlayer.Character and LocalPlayer.Character:FindFirstChild("Humanoid")
        local root = getRoot(LocalPlayer.Character)
        if not humanoid or humanoid.Health <= 0 or not root then break end
        chaoticVoidTeleport()
        task.wait(manualOrbitRate)
        if math.random() < 0.15 then
            task.wait(0.15 + math.random() * 0.2)
        end
    end
    voidSpamCoroutine = nil
end

local function updateVoidSpam()
    if voidSpamCoroutine then
        task.cancel(voidSpamCoroutine)
        voidSpamCoroutine = nil
    end
    if voidSpamEnabled then
        if chaseModeEnabled then
            voidSpamCoroutine = task.spawn(chaseVoidSpamLoop)
        else
            voidSpamCoroutine = task.spawn(normalVoidSpamLoop)
        end
    end
end

local function toggleVoidSpam(state)
    voidSpamEnabled = state
    updateVoidSpam()
end

local function toggleChaseMode(state)
    chaseModeEnabled = state
    updateVoidSpam()
end

-- =============================
-- ⚔️ ANTI KICIA
-- =============================
local function teleportOnceKicia()
    local root = getRoot(LocalPlayer.Character)
    if not root then return end
    local pos = root.Position
    local angle = math.random() * math.pi * 2
    local dist = math.random(500, 5000)
    safeTeleport(Vector3.new(
        pos.X + math.cos(angle) * dist,
        pos.Y + 100,
        pos.Z + math.sin(angle) * dist
    ))
end

local function toggleAntiKicia(state)
    antiKiciaEnabled = state
    if antiKiciaEnabled then
        if not antiKiciaConnection then
            antiKiciaConnection = RunService.Heartbeat:Connect(function()
                teleportOnceKicia()
                task.wait(0.1)
            end)
        end
    else
        if antiKiciaConnection then
            antiKiciaConnection:Disconnect()
            antiKiciaConnection = nil
        end
    end
end

-- =============================
-- 🌍 ORBIT FAR (Your Style — Fully Randomized Every Frame)
-- =============================
local function orbitTeleport()
    local opponentHRP = getClosestOpponentHRP()
    if not opponentHRP then return end

    local center = opponentHRP.Position
    local angle = math.random() * math.pi * 2
    local distance = math.random(150000, 200000)  -- 🔥 Random distance
    local heightOffset = math.random(-25, 25)     -- 🔥 Random height offset
    local height = 10000 + heightOffset           -- Base height: 10k

    local target = center + Vector3.new(
        math.cos(angle) * distance,
        height,
        math.sin(angle) * distance
    )

    safeTeleport(target)
end

local function toggleOrbitFar(state)
    orbitFarEnabled = state
    if orbitFarEnabled then
        if not orbitFarConnection then
            orbitFarConnection = RunService.Heartbeat:Connect(orbitTeleport)
        end
    else
        if orbitFarConnection then
            orbitFarConnection:Disconnect()
            orbitFarConnection = nil
        end
    end
end

-- =============================
-- ⌨️ KEYBIND HANDLERS
-- =============================
UserInputService.InputBegan:Connect(function(input, gp)
    if gp then return end
    if input.KeyCode == antiKiciaKey then
        toggleAntiKicia(not antiKiciaEnabled)
    elseif input.KeyCode == orbitFarKey then
        toggleOrbitFar(not orbitFarEnabled)
    end
end)

-- =============================
-- ♻️ RESPAWN HANDLER
-- =============================
LocalPlayer.CharacterAdded:Connect(function(char)
    task.wait(0.5)
    if voidSpamEnabled then updateVoidSpam() end
    if autoTeleportEnabled then toggleAutoTeleport(true) end
    if antiKiciaEnabled then toggleAntiKicia(true) end
    if orbitFarEnabled then toggleOrbitFar(true) end
end)

-- =============================
-- 🖥️ RAYFIELD UI
-- =============================

Tab:CreateToggle({ Name = "Voidspam+", CurrentValue = false, Callback = toggleVoidSpam })
Tab:CreateToggle({ Name = "Chase Mode", CurrentValue = false, Callback = toggleChaseMode })
Tab:CreateToggle({ Name = "Targetstrafe Resolver", CurrentValue = false, Callback = toggleAutoTeleport })
Tab:CreateToggle({ Name = "Anti Kicia", CurrentValue = false, Callback = toggleAntiKicia })
Tab:CreateToggle({ Name = "Orbit Far (150k–200k)", CurrentValue = false, Callback = toggleOrbitFar })

-- Keybind Dropdown
local keyOptions = {}
for _, k in pairs(Enum.KeyCode:GetEnumItems()) do
    keyOptions[k.Name] = k
end

Tab:CreateDropdown({
    Name = "Anti Kicia Key",
    Options = keyOptions,
    CurrentOption = {"P"},
    Callback = function(sel)
        antiKiciaKey = keyOptions[sel[1]]
    end
})

-- Manual Controls
Tab:CreateToggle({ Name = "Use Manual Distance", CurrentValue = false, Callback = function(v) useManualDistance = v end })
Tab:CreateSlider({ Name = "Orbit Distance (Studs)", Range = {5, 100}, Increment = 1, Suffix = " studs", CurrentValue = manualDistanceOverride, Callback = function(v) manualDistanceOverride = v end })
Tab:CreateSlider({ Name = "Orbit Rate (Delay)", Range = {0.01, 1.0}, Increment = 0.01, Suffix = " sec", CurrentValue = manualOrbitRate, Callback = function(v) manualOrbitRate = v; orbitRate = v end })

-- =============================
-- ✅ NOTIFICATION
-- =============================
Rayfield:Notify({
    Title = "Combat Hub Loaded",
    Content = "Voidspam+, Targetstrafe, Anti Kicia, Orbit Far!",
    Duration = 4,
    Image = 4483362458
})