Config = table
Config.Name = ""
Config.Pass = ""
Include ("lua/koglvl.config")


SetScriptTitle("KoG|lvl")
SetScriptInfo("(c) by MAP94")
SetScriptUseSettingPage(0)

AddEventListener("OnChat", "Chat")


Ui = table

Ui.ActivExp = false
Ui.ExpRect = nil
Ui.ExpBar = nil
Ui.ExpLabel = nil

Ui.ExpEffektID = {}
Ui.ExpEffektActiv = {}
for i = 1, 16 do
    Ui.ExpEffektID[i] = 0
    Ui.ExpEffektActiv[i] = false
end


Ui.ActivLogin = false
Ui.LoginRect = nil
Ui.LoginNameLabel = nil
Ui.LoginNameBox = nil
Ui.LoginPassLabel = nil
Ui.LoginPassBox = nil
Ui.LoginButton = nil


Ui.ActivInGame = false
Ui.InGameRect = nil
Ui.InGameLogoutButton = nil
Ui.InGameName = nil
Ui.InGameUpgrade = nil
Ui.InGameHammerButton = nil
Ui.InGameGunButton = nil
Ui.InGameShotgunButton = nil
Ui.InGameGrenadeButton = nil
Ui.InGameRifleButton = nil
Ui.InGameLifeButton = nil
Ui.InGameHandleButton = nil
Ui.InGameHammerLabel = nil
Ui.InGameGunLabel = nil
Ui.InGameShotgunLabel = nil
Ui.InGameGrenadeLabel = nil
Ui.InGameRifleLabel = nil
Ui.InGameLifeLabel = nil
Ui.InGameHandleLabel = nil
Ui.InGameMoneyLabel = nil
Ui.InGameMoneyLabelValue = nil

IsLoggedIn = false
BarNeedRefresh = false
InGameNeedRefresh = true
StatsNeedRefresh = false

NowExp = ""
FullExp = ""
StatHammer = 0
StatGun = 0
StatShotgun = 0
StatGrenade = 0
StatRifle = 0
StatLife = 0
StatHandle = 0
StatLevel = 0
StatMoney = 0
function Chat()
    if (ChatGetClientID() == -1) then
        Text = ChatGetText()
        NumSplitter = 0
        if (Text == "Logged in successfully") then
            ChatSend("/stats")
            IsLoggedIn = true
        end
        if (Text == "Logged out successfully!") then
            IsLoggedIn = false
        end
        if (string.char(Text:byte(1)) == "H" and string.char(Text:byte(2)) == "a" and string.char(Text:byte(3)) == "m") then
            ChatHide()
            i = 0
            for w in Text:gmatch("%d+") do
                i = i + 1
                if (i == 1) then
                    StatHammer = tonumber(w)
                end
                if (i == 2) then
                    StatGun = tonumber(w)
                end
                if (i == 3) then
                    StatShotgun = tonumber(w)
                end
                if (i == 4) then
                    StatGrenade = tonumber(w)
                end
            end
        end
        if (string.char(Text:byte(1)) == "R" and string.char(Text:byte(2)) == "i" and string.char(Text:byte(3)) == "f") then
            ChatHide()
            i = 0
            for w in Text:gmatch("%d+") do
                i = i + 1
                if (i == 1) then
                    StatRifle = tonumber(w)
                end
                if (i == 2) then
                    StatLife = tonumber(w)
                end
                if (i == 3) then
                    StatHandle = tonumber(w)
                end
            end
        end
        if (string.char(Text:byte(1)) == "L" and string.char(Text:byte(2)) == "e" and string.char(Text:byte(3)) == "v") then
            ChatHide()
            StatsNeedRefresh = true
            i = 0
            for w in Text:gmatch("%d+") do
                i = i + 1
                if (i == 1) then
                    StatLevel = tonumber(w)
                end
                if (i == 2) then
                    StatMoney = tonumber(w)
                end
            end
        end
        if (string.char(Text:byte(1)) == "/" and string.char(Text:byte(2)) == "u" and string.char(Text:byte(3)) == "p") then
            ChatHide()
            ChatSend("/stats")
        end
        if (string.char(Text:byte(1)) == "/" and string.char(Text:byte(2)) == "s" and string.char(Text:byte(3)) == "t") then
            ChatHide()
        end
        if (string.char(Text:byte(1)) == "E" and string.char(Text:byte(2)) == "x" and string.char(Text:byte(3)) == "p") then
            ChatHide()
            TmpNowExp = ""
            TmpFullExp = ""
            for i = 0, Text:len() do
                if (NumSplitter == 2 and string.char(Text:byte(i)) ~= "/" and string.char(Text:byte(i)) ~= " ") then
                    TmpNowExp = TmpNowExp .. string.char(Text:byte(i))
                end
                if (NumSplitter == 3) then
                    TmpFullExp = TmpFullExp .. string.char(Text:byte(i))
                end
                if (string.char(Text:byte(i)) == "|" or string.char(Text:byte(i)) == "/") then
                    if (string.char(Text:byte(i)) == "|") then
                        i = i + 1
                    end
                    NumSplitter = NumSplitter + 1
                end
            end
            LastExp = NowExp
            NowExp = TmpNowExp
            FullExp = TmpFullExp
            BarNeedRefresh = true
            for i = 1, 16 do
                if (Ui.ExpEffektActiv[i] == false) then
                    Width = UiGetScreenHeight() * 0.2
                    Height = UiGetScreenHeight() * 0.04
                    y = UiGetScreenHeight() * 0.05 * 2
                    Print("x", tonumber(NowExp) - tonumber(LastExp))
                    if (tonumber(NowExp) - tonumber(LastExp) == 51) then
                        Ui.ExpEffektID[i] = UiDoLabel(UiGetScreenWidth() / 2 - Width / 2, y, Width, Height, 0, "+50", 50, 0, 1, 0, 0, 1)
                    elseif (FullExp == NowExp) then
                        Ui.ExpEffektID[i] = UiDoLabel(UiGetScreenWidth() / 2 - Width / 2, y, Width, Height, 0, "Level up", 50, 0)
                    else
                        Ui.ExpEffektID[i] = UiDoLabel(UiGetScreenWidth() / 2 - Width / 2, y, Width, Height, 0, "+1", 25, 0)
                    end
                    Ui.ExpEffektActiv[i] = true
                    break
                end
            end
        end
    end
end

function Login()
    ChatSend("/login " .. UiGetText(Ui.LoginNameBox) .. " " .. UiGetText(Ui.LoginPassBox))
    configout = io.open("lua/koglvl.config", "wb")
    configout:write("Config.Name = \"" .. UiGetText(Ui.LoginNameBox) .. "\"\n")
    configout:write("Config.Pass = \"" .. UiGetText(Ui.LoginPassBox) .. "\"\n")
    configout:close()
    Include ("lua/koglvl.config")
end

function Logout()
    ChatSend("/logout")
    IsLoggedIn = false
end

function UpgrHammer()
    ChatSend("/upgr hammer")
    ChatSend("/stats")
end

function UpgrGun()
    ChatSend("/upgr gun")
    ChatSend("/stats")
end

function UpgrShotgun()
    ChatSend("/upgr shotgun")
    ChatSend("/stats")
end

function UpgrGrenade()
    ChatSend("/upgr grenade")
    ChatSend("/stats")
end

function UpgrRifle()
    ChatSend("/upgr rifle")
    ChatSend("/stats")
end

function UpgrLife()
    ChatSend("/upgr life")
    ChatSend("/stats")
end

function UpgrHandle()
    ChatSend("/upgr handle")
    ChatSend("/stats")
end

function IsKogServer()
    if (GetGameType() == "KoG|DM") then
        return true
    end
    if (GetGameType() == "KoG|TDM") then
        return true
    end
    if (GetGameType() == "KoG|CTF") then
        return true
    end
    return false
end

iTick = 0
function Tick(Time, ServerTick)
    if (StateOnline() == false) then
        IsLoggedIn = false
    end
    if (Ui.ActivExp == true and ((StateOnline() and MenuActiv() == false and IsKogServer()) == false or BarNeedRefresh)) then
        UiRemoveElement(Ui.ExpRect)
        UiRemoveElement(Ui.ExpBar)
        UiRemoveElement(Ui.ExpLabel)
        Ui.ActivExp = false;
    end
    if (Ui.ActivLogin == true and ((StateOnline() and MenuGameActiv() and IsKogServer()) == false or IsLoggedIn)) then
        UiRemoveElement(Ui.LoginRect)
        UiRemoveElement(Ui.LoginNameLabel)
        UiRemoveElement(Ui.LoginNameBox)
        UiRemoveElement(Ui.LoginPassLabel)
        UiRemoveElement(Ui.LoginPassBox)
        UiRemoveElement(Ui.LoginButton)
        Ui.ActivLogin = false
    end
    if (Ui.ActivInGame == true and ((StateOnline() and MenuGameActiv() and IsKogServer()) == false or StatsNeedRefresh)) then
        UiRemoveElement(Ui.InGameRect)
        UiRemoveElement(Ui.InGameName)
        UiRemoveElement(Ui.InGameLogoutButton)
        UiRemoveElement(Ui.InGameUpgrade)
        UiRemoveElement(Ui.InGameHammerButton)
        UiRemoveElement(Ui.InGameHammerLabel)
        UiRemoveElement(Ui.InGameGunButton)
        UiRemoveElement(Ui.InGameGunLabel)
        UiRemoveElement(Ui.InGameShotgunButton)
        UiRemoveElement(Ui.InGameShotgunLabel)
        UiRemoveElement(Ui.InGameGrenadeButton)
        UiRemoveElement(Ui.InGameGrenadeLabel)
        UiRemoveElement(Ui.InGameRifleButton)
        UiRemoveElement(Ui.InGameRifleLabel)
        UiRemoveElement(Ui.InGameLifeButton)
        UiRemoveElement(Ui.InGameLifeLabel)
        UiRemoveElement(Ui.InGameHandleButton)
        UiRemoveElement(Ui.InGameHandleLabel)
        UiRemoveElement(Ui.InGameMoneyLabel)
        UiRemoveElement(Ui.InGameMoneyLabelValue)
        Ui.ActivInGame = false
    end
    for i = 1, 16 do
        if (Ui.ExpEffektActiv[i] == true) then
            s = UiGetScreenHeight() * 0.05 * 0.01
            r, g, b, a = UiGetColor(Ui.ExpEffektID[i])
            x, y, w, h = UiGetRect(Ui.ExpEffektID[i])
            a = a - 0.01
            y = y - s
            UiSetColor(Ui.ExpEffektID[i], r, g, b, a)
            UiSetRect(Ui.ExpEffektID[i], x, y, w, h)
            if (a <= 0) then
                UiRemoveElement(Ui.ExpEffektID[i])
                Ui.ExpEffektActiv[i] = false
            end
        end
    end
    if (StateOnline() and MenuActiv() == false and IsKogServer() and Ui.ActivExp == false and IsLoggedIn == true) then
        Ui.ActivExp = true
        Width = UiGetScreenHeight() * 0.2
        Height = UiGetScreenHeight() * 0.04
        Spacing = (UiGetScreenHeight() * 0.2 * 0.03)
        BarWidth = Width - Spacing * 2
        BarHeight = Height - Spacing * 2
        y = UiGetScreenHeight() * 0.05
        Ui.ExpRect = UiDoRect(UiGetScreenWidth() / 2 - Width / 2, y, Width, Height, 0, 15, 5, 0, 0, 0, 0.5)
        Ui.ExpBar = UiDoRect(UiGetScreenWidth() / 2 - Width / 2 + Spacing, y + Spacing, BarWidth * (tonumber(NowExp) / tonumber(FullExp)), BarHeight, 0, 15, 3, 0, 1, 0, 0.8)
        if (FullExp == NowExp) then
            Ui.ExpLabel = UiDoLabel(UiGetScreenWidth() / 2 - Width / 2, y + Spacing, Width, BarHeight, 0, "Level up!", 14, 0)
        else
            Ui.ExpLabel = UiDoLabel(UiGetScreenWidth() / 2 - Width / 2, y + Spacing, Width, BarHeight, 0, tonumber(FullExp) - tonumber(NowExp), 14, 0)
        end
        BarNeedRefresh = false
    end
    if (StateOnline() and MenuGameActiv() and IsKogServer() and Ui.ActivLogin == false and IsLoggedIn == false) then
        Ui.ActivLogin = true
        Width = UiGetScreenHeight() * 0.5
        Height = UiGetScreenHeight() * 0.14
        x = UiGetScreenWidth() / 2 - Width / 2
        y = UiGetScreenHeight() / 2 - Height / 2
        Spacing = (UiGetScreenHeight() * 0.2 * 0.05)
        Ui.LoginRect = UiDoRect(x, y, Width, Height, 0, 15, 15, 0, 0, 0, 0.5)
        x = x + Spacing
        y = y + Spacing
        Ui.LoginNameLabel = UiDoLabel(x, y, Width / 3 - Spacing * 2, Height / 3 - Spacing * 1.3, 0, "Username:", 17, 1)
        Ui.LoginNameBox = UiDoEditBox(x + Width / 3, y, Width / 3 * 2 - Spacing * 2, Height / 3 - Spacing * 1.3, 0, Config.Name, 17)
        Ui.LoginPassLabel = UiDoLabel(x, y + Height / 3 - Spacing / 1.5, Width / 3 - Spacing * 2, Height / 3 - Spacing * 1.3, 0, "Password:", 17, 1)
        Ui.LoginPassBox = UiDoEditBox(x + Width / 3, y + Height / 3 - Spacing / 1.5, Width / 3 * 2 - Spacing * 2, Height / 3 - Spacing * 1.3, 0, Config.Pass, 17, 1)
        Ui.LoginButton = UiDoButton(x, y + Height * 2 / 3 - Spacing, Width - Spacing * 2, Height / 3 - Spacing, 0, "Login", "Login")
    end
    if (IsLoggedIn and InGameNeedRefresh) then
        InGameNeedRefresh = false
        ChatSend("/stats")
    end
    if (StateOnline() and MenuGameActiv() and IsKogServer() and Ui.ActivInGame == false and IsLoggedIn == true) then
        Ui.ActivInGame = true
        Spacing = UiGetScreenHeight() * 0.2 * 0.05
        Width = UiGetScreenHeight() * 0.5 + Spacing * 2
        Height = Spacing * 40 + Spacing * 2
        x = UiGetScreenWidth() / 2 - Width / 2
        y = UiGetScreenHeight() / 2 - Height / 2
        Ui.InGameRect = UiDoRect(x, y, Width, Height, 0, 15, 15, 0, 0, 0, 0.5)
        x = x + Spacing
        y = y + Spacing
        Width = Width - Spacing * 2
        Height = Height - Spacing * 2
        Ui.InGameName = UiDoLabel(x, y, Width / 3 * 2 - Spacing, Spacing * 3, 0, "You are logged in as: " .. Config.Name, 16, -1)
        Ui.InGameLogoutButton = UiDoButton(x + Width / 3 * 2, y, Width / 3 - Spacing, Spacing * 3, 0, "Logout", "Logout")
        Ui.InGameUpgrade = UiDoLabel(x, y + Spacing * 4, Width, Spacing * 3, 0, "Upgrade:", 16, 0)

        Ui.InGameMoneyLabel = UiDoLabel(x, y + Spacing * 8, Width / 2 - Spacing, Spacing * 3, 0, "Money", 16, 0)
        Ui.InGameMoneyLabelValue = UiDoLabel(x + Width / 2, y + Spacing * 8, Width / 2 - Spacing, Spacing * 3, 0, StatMoney, 14, -1)

        Ui.InGameHammerButton = UiDoButton(x, y + Spacing * 12, Width / 2 - Spacing, Spacing * 3, 0, "Hammer", "UpgrHammer")
        Ui.InGameHammerLabel = UiDoLabel(x + Width / 2, y + Spacing * 12, Width / 2 - Spacing, Spacing * 3, 0, StatHammer, 14, -1)

        Ui.InGameGunButton = UiDoButton(x, y + Spacing * 16, Width / 2 - Spacing, Spacing * 3, 0, "Gun", "UpgrGun")
        Ui.InGameGunLabel = UiDoLabel(x + Width / 2, y + Spacing * 16, Width / 2 - Spacing, Spacing * 3, 0, StatGun, 14, -1)

        Ui.InGameShotgunButton = UiDoButton(x, y + Spacing * 20, Width / 2 - Spacing, Spacing * 3, 0, "Shotgun", "UpgrShotgun")
        Ui.InGameShotgunLabel = UiDoLabel(x + Width / 2, y + Spacing * 20, Width / 2 - Spacing, Spacing * 3, 0, StatShotgun, 14, -1)

        Ui.InGameGrenadeButton = UiDoButton(x, y + Spacing * 24, Width / 2 - Spacing, Spacing * 3, 0, "Grenade", "UpgrGrenade")
        Ui.InGameGrenadeLabel = UiDoLabel(x + Width / 2, y + Spacing * 24, Width / 2 - Spacing, Spacing * 3, 0, StatGrenade, 14, -1)

        Ui.InGameRifleButton = UiDoButton(x, y + Spacing * 28, Width / 2 - Spacing, Spacing * 3, 0, "Rifle", "UpgrRifle")
        Ui.InGameRifleLabel = UiDoLabel(x + Width / 2, y + Spacing * 28, Width / 2 - Spacing, Spacing * 3, 0, StatRifle, 14, -1)

        Ui.InGameLifeButton = UiDoButton(x, y + Spacing * 32, Width / 2 - Spacing, Spacing * 3, 0, "Life", "UpgrLife")
        Ui.InGameLifeLabel = UiDoLabel(x + Width / 2, y + Spacing * 32, Width / 2 - Spacing, Spacing * 3, 0, StatLife, 14, -1)

        Ui.InGameHandleButton = UiDoButton(x, y + Spacing * 36, Width / 2 - Spacing, Spacing * 3, 0, "Handle", "UpgrHandle")
        Ui.InGameHandleLabel = UiDoLabel(x + Width / 2, y + Spacing * 36, Width / 2 - Spacing, Spacing * 3, 0, StatHandle, 14, -1)
    end
end
