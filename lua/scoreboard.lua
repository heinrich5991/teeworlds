--Todos: Flags, Katana, Reset
SetScriptTitle("Scoreboard")
SetScriptInfo("(c) by MAP94")
SetScriptUseSettingPage(0)

AddEventListener("OnScoreboardRender", "Test")
AddEventListener("OnKill", "Kill")

Ui = table
Ui.Activ = false
Ui.BaseRect = nil
Ui.Highlight = {}
Ui.CountryFlag = {}
Ui.Name = {}
Ui.Ping = {}
Ui.Clan = {}
Ui.Score = {}
Ui.Frags = {}
Ui.Deaths = {}
Ui.Ratio = {}
Ui.Fpm = {}
Ui.Hammer = {}
Ui.Gun = {}
Ui.Shotgun = {}
Ui.Grenade = {}
Ui.Rifle = {}
Ui.Katana = {}
Ui.Flags = {}
Ui.ID = {}

function CorrectValue(i)
    return i / 1200 * UiGetScreenHeight()
end

Players = {}
for i = 1, 16 do
    Players[i] = {}
    Players[i]["frags"] = {}
    Players[i]["frags"]["all"] = 0
    Players[i]["frags"][0] = 0
    Players[i]["frags"][1] = 0
    Players[i]["frags"][2] = 0
    Players[i]["frags"][3] = 0
    Players[i]["frags"][4] = 0
    Players[i]["deaths"] = {}
    Players[i]["deaths"]["all"] = 0
    Players[i]["deaths"][0] = 0
    Players[i]["deaths"][1] = 0
    Players[i]["deaths"][2] = 0
    Players[i]["deaths"][3] = 0
    Players[i]["deaths"][4] = 0
    Players[i]["clientid"] = 0
    Players[i]["team"] = 0
end

StartTime = nil
ActualTime = nil

function Kill()
    Killer = KillGetKillerID()
    Victim = KillGetVictimID()
    Weapon = KillGetWeapon()
    if (Killer ~= Victim) then
        Players[Killer + 1]["frags"][Weapon] = Players[Killer + 1]["frags"][Weapon] + 1
    end
    Players[Victim + 1]["deaths"][Weapon] = Players[Victim + 1]["deaths"][Weapon] + 1

    if (Killer ~= Victim) then
        Players[Killer + 1]["frags"]["all"] = Players[Killer + 1]["frags"]["all"] + 1
    end
    Players[Victim + 1]["deaths"]["all"] = Players[Victim + 1]["deaths"]["all"] + 1
end

function round(num, idp)
  local mult = 10^(idp or 0)
  return math.floor(num * mult + 0.5) / mult
end

function SortTeamScore(a, b)
    if (a["score"] ~= nil) then
        if (b["score"] == nil) then
            return true
        else
            if (a["team"] ~= b["team"]) then
                if (a["team"] > b["team"]) then
                    return true
                else
                    return false
                end
            elseif (a["score"] > b["score"]) then
                return true
            end
        end
    end
    return false
end

function deepcopy(t)
    if type(t) ~= 'table' then
        return t
    end
    local mt = getmetatable(t)
    local res = {}
    for k,v in pairs(t) do
        if type(v) == 'table' then
            v = deepcopy(v)
        end
        res[k] = v
    end
    setmetatable(res,mt)
    return res
end

function Test()
    ScoreboardAbortRender()
    if (MenuActiv() == false) then
        Width = UiGetScreenWidth()
        Height = UiGetScreenHeight()
        w = CorrectValue(1600)
        h = CorrectValue(760)
        x = Width / 2 - w / 2
        y = CorrectValue(150)

        for i = 1, 16 do
            Players[i]["name"] = GetPlayerName(i - 1)
            if (Players[i]["name"] ~= nil and Players[i]["name"] ~= "") then
                Players[i]["clan"] = GetPlayerClan(i - 1)
                Players[i]["country"] = GetPlayerCountry(i - 1)
                Players[i]["score"] = GetPlayerScore(i - 1)
                Players[i]["ping"] = GetPlayerPing(i - 1)
                Players[i]["clientid"] = i - 1
                Players[i]["team"] = GetPlayerTeam(i - 1)
            else
                Players[i]["name"] = ""
                Players[i]["clan"] = ""
                Players[i]["country"] = 0
                Players[i]["ping"] = 0
                Players[i]["score"] = nil
                Players[i]["frags"] = {}
                Players[i]["frags"]["all"] = 0
                Players[i]["frags"][0] = 0
                Players[i]["frags"][1] = 0
                Players[i]["frags"][2] = 0
                Players[i]["frags"][3] = 0
                Players[i]["frags"][4] = 0
                Players[i]["deaths"] = {}
                Players[i]["deaths"]["all"] = 0
                Players[i]["deaths"][0] = 0
                Players[i]["deaths"][1] = 0
                Players[i]["deaths"][2] = 0
                Players[i]["deaths"][3] = 0
                Players[i]["deaths"][4] = 0
            end
        end
        PlayersSorted = deepcopy(Players)
        table.sort(PlayersSorted, SortTeamScore)

        Ui.Activ = true

        Ui.BaseRect = UiDoRect(x, y, w, h, 0, 15, CorrectValue(17), 0, 0, 0, 0.5)
        y = y + 5
        x = x + 5
        Ui.Score[0] = UiDoLabel(x, y, 35, 35, 0, "Score", 14, -1)
        Ui.Name[0] = UiDoLabel(x + 45, y, 100, 30, 0, "Name", 16, -1)
        Ui.Clan[0] = UiDoLabel(x + 150, y, 45, 30, 0, "Clan", 14, -1)
        Ui.Ping[0] = UiDoLabel(x + 250, y, 45, 30, 0, "Ping", 14, -1)
        Ui.Frags[0] = UiDoLabel(x + 300, y, 45, 30, 0, "Frags", 14, -1)
        Ui.Deaths[0] = UiDoLabel(x + 350, y, 45, 30, 0, "Deaths", 14, -1)
        Ui.Ratio[0] = UiDoLabel(x + 400, y, 45, 30, 0, "Ratio", 14, -1)
        Ui.Fpm[0] = UiDoLabel(x + 450, y, 45, 30, 0, "FPM", 14, -1)
        Ui.Hammer[0] = UiDoImage(x + 500 + 11, y, 22, 15, 0, UiGetGameTextureID(), 41, "")
        Ui.Gun[0] = UiDoImage(x + 550 + 13, y, 26, 15, 0, UiGetGameTextureID(), 26, "")
        Ui.Shotgun[0] = UiDoImage(x + 600, y, 44, 15, 0, UiGetGameTextureID(), 32, "")
        Ui.Grenade[0] = UiDoImage(x + 650, y, 44, 15, 0, UiGetGameTextureID(), 38, "")
        Ui.Rifle[0] = UiDoImage(x + 700, y, 44, 17, 0, UiGetGameTextureID(), 47, "")
        Ui.ID[0] = UiDoLabel(x + 750, y, 44, 17, 0, "ID", 14, 0)
        for i = 1, 16 do
            if (PlayersSorted[i]["score"] ~= nil) then
                if (IsTeamplay() == true) then
                    if (PlayersSorted[i]["clientid"] == GetLocalCharacterId()) then
                        if (PlayersSorted[i]["team"] == 0) then
                            Ui.Highlight[i] = UiDoRect(x, y + (i * 20), w - 10, 18, 0, 15, CorrectValue(5), 1, 0, 0, 0.5)
                        elseif (PlayersSorted[i]["team"] == 1) then
                            Ui.Highlight[i] = UiDoRect(x, y + (i * 20), w - 10, 18, 0, 15, CorrectValue(5), 0, 0, 1, 0.5)
                        else
                            Ui.Highlight[i] = UiDoRect(x, y + (i * 20), w - 10, 18, 0, 15, CorrectValue(5), 1, 1, 1, 0.25)
                        end
                    else
                        if (PlayersSorted[i]["team"] == 0) then
                            Ui.Highlight[i] = UiDoRect(x, y + (i * 20), w - 10, 18, 0, 15, CorrectValue(5), 1, 0, 0, 0.25)
                        elseif (PlayersSorted[i]["team"] == 1) then
                            Ui.Highlight[i] = UiDoRect(x, y + (i * 20), w - 10, 18, 0, 15, CorrectValue(5), 0, 0, 1, 0.25)
                        end
                    end
                else
                    if (PlayersSorted[i]["clientid"] == GetLocalCharacterId()) then
                        Ui.Highlight[i] = UiDoRect(x, y + (i * 20), w - 10, 18, 0, 15, CorrectValue(5), 1, 1, 1, 0.25)
                    end
                end
                if (PlayersSorted[i]["team"] == -1) then
                    Ui.Score[i] = UiDoLabel(x, y + (i * 20), 35, 20, 0, PlayersSorted[i]["score"], 14, 1, 0.5, 0.5, 0.5, 1)
                    Ui.Name[i] = UiDoLabel(x + 45, y + (i * 20), 100, 20, 0, PlayersSorted[i]["name"], 14, -1, 0.5, 0.5, 0.5, 1)
                    Ui.Clan[i] = UiDoLabel(x + 150, y + (i * 20), 100, 20, 0, PlayersSorted[i]["clan"], 14, -1, 0.5, 0.5, 0.5, 1)
                    Ui.CountryFlag[i] = UiDoImage(x + 200, y + (i * 20) + 1, 32, 16, 0, UiGetFlagTextureID(PlayersSorted[i]["country"]), -1, "")
                    Ui.Ping[i] = UiDoLabel(x + 250, y + (i * 20), 45, 20, 0, PlayersSorted[i]["ping"], 14, -1, 0.5, 0.5, 0.5, 1)
                    Ui.Frags[i] = UiDoLabel(x + 300, y + (i * 20), 45, 20, 0, PlayersSorted[i]["frags"]["all"], 14, -1, 0.5, 0.5, 0.5, 1)
                    Ui.Deaths[i] = UiDoLabel(x + 350, y + (i * 20), 45, 20, 0, PlayersSorted[i]["deaths"]["all"], 14, -1, 0.5, 0.5, 0.5, 1)
                    if (PlayersSorted[i]["deaths"]["all"] == 0) then
                        Ui.Ratio[i] = UiDoLabel(x + 400, y + (i * 20), 45, 20, 0, "-.--", 14, -1, 0.5, 0.5, 0.5, 1)
                    else
                        Ui.Ratio[i] = UiDoLabel(x + 400, y + (i * 20), 45, 20, 0, round(PlayersSorted[i]["frags"]["all"] / PlayersSorted[i]["deaths"]["all"], 2), 14, -1, 0.5, 0.5, 0.5, 1)
                    end
                    Ui.Fpm[i] = UiDoLabel(x + 450, y + (i * 20), 45, 20, 0, round(PlayersSorted[i]["frags"]["all"] / ((ActualTime - StartTime) / 600), 2), 14, -1, 0.5, 0.5, 0.5, 1)
                    Ui.Hammer[i] = UiDoLabel(x + 500, y + (i * 20), 45, 20, 0, PlayersSorted[i]["frags"][0] .. "/" .. PlayersSorted[i]["deaths"][0], 14, 0, 0.5, 0.5, 0.5, 1)
                    Ui.Gun[i] = UiDoLabel(x + 550, y + (i * 20), 45, 20, 0, PlayersSorted[i]["frags"][1] .. "/" .. PlayersSorted[i]["deaths"][1], 14, 0, 0.5, 0.5, 0.5, 1)
                    Ui.Shotgun[i] = UiDoLabel(x + 600, y + (i * 20), 45, 20, 0, PlayersSorted[i]["frags"][2] .. "/" .. PlayersSorted[i]["deaths"][2], 14, 0, 0.5, 0.5, 0.5, 1)
                    Ui.Grenade[i] = UiDoLabel(x + 650, y + (i * 20), 45, 20, 0, PlayersSorted[i]["frags"][3] .. "/" .. PlayersSorted[i]["deaths"][3], 14, 0, 0.5, 0.5, 0.5, 1)
                    Ui.Rifle[i] = UiDoLabel(x + 700, y + (i * 20), 45, 20, 0, PlayersSorted[i]["frags"][4] .. "/" .. PlayersSorted[i]["deaths"][4], 14, 0, 0.5, 0.5, 0.5, 1)
                    Ui.ID[i] = UiDoLabel(x + 750, y + (i * 20), 45, 20, 0, PlayersSorted[i]["clientid"], 14, 0, 0.5, 0.5, 0.5, 1)
                else
                    Ui.Score[i] = UiDoLabel(x, y + (i * 20), 35, 20, 0, PlayersSorted[i]["score"], 14, 1)
                    Ui.Name[i] = UiDoLabel(x + 45, y + (i * 20), 100, 20, 0, PlayersSorted[i]["name"], 14, -1)
                    Ui.Clan[i] = UiDoLabel(x + 150, y + (i * 20), 100, 20, 0, PlayersSorted[i]["clan"], 14, -1)
                    Ui.CountryFlag[i] = UiDoImage(x + 200, y + (i * 20) + 1, 32, 16, 0, UiGetFlagTextureID(PlayersSorted[i]["country"]), -1, "")
                    Ui.Ping[i] = UiDoLabel(x + 250, y + (i * 20), 45, 20, 0, PlayersSorted[i]["ping"], 14, -1)
                    Ui.Frags[i] = UiDoLabel(x + 300, y + (i * 20), 45, 20, 0, PlayersSorted[i]["frags"]["all"], 14, -1)
                    Ui.Deaths[i] = UiDoLabel(x + 350, y + (i * 20), 45, 20, 0, PlayersSorted[i]["deaths"]["all"], 14, -1)
                    if (PlayersSorted[i]["deaths"]["all"] == 0) then
                        Ui.Ratio[i] = UiDoLabel(x + 400, y + (i * 20), 45, 20, 0, "-.--", 14, -1)
                    else
                        Ui.Ratio[i] = UiDoLabel(x + 400, y + (i * 20), 45, 20, 0, round(PlayersSorted[i]["frags"]["all"] / PlayersSorted[i]["deaths"]["all"], 2), 14, -1)
                    end
                    Ui.Fpm[i] = UiDoLabel(x + 450, y + (i * 20), 45, 20, 0, round(PlayersSorted[i]["frags"]["all"] / ((ActualTime - StartTime) / 600), 2), 14, -1)
                    Ui.Hammer[i] = UiDoLabel(x + 500, y + (i * 20), 45, 20, 0, PlayersSorted[i]["frags"][0] .. "/" .. PlayersSorted[i]["deaths"][0], 14, 0)
                    Ui.Gun[i] = UiDoLabel(x + 550, y + (i * 20), 45, 20, 0, PlayersSorted[i]["frags"][1] .. "/" .. PlayersSorted[i]["deaths"][1], 14, 0)
                    Ui.Shotgun[i] = UiDoLabel(x + 600, y + (i * 20), 45, 20, 0, PlayersSorted[i]["frags"][2] .. "/" .. PlayersSorted[i]["deaths"][2], 14, 0)
                    Ui.Grenade[i] = UiDoLabel(x + 650, y + (i * 20), 45, 20, 0, PlayersSorted[i]["frags"][3] .. "/" .. PlayersSorted[i]["deaths"][3], 14, 0)
                    Ui.Rifle[i] = UiDoLabel(x + 700, y + (i * 20), 45, 20, 0, PlayersSorted[i]["frags"][4] .. "/" .. PlayersSorted[i]["deaths"][4], 14, 0)
                    Ui.ID[i] = UiDoLabel(x + 750, y + (i * 20), 45, 20, 0, PlayersSorted[i]["clientid"], 14, 0)
                end
            end
        end
    end
end

iTick = 0
function Tick(Time, ServerTick)
    iTick = iTick + 1
    Time = Time or 0
    ServerTick = ServerTick or 0
    if (StartTime == nil) then
        StartTime = Time
    end
        ActualTime = Time


    if (Ui.Activ == true) then
        UiRemoveElement(Ui.BaseRect)

        for i = 0, 16 do
            UiRemoveElement(Ui.Name[i])
            UiRemoveElement(Ui.Highlight[i])
            UiRemoveElement(Ui.CountryFlag[i])
            UiRemoveElement(Ui.Score[i])
            UiRemoveElement(Ui.Clan[i])
            UiRemoveElement(Ui.Ping[i])
            UiRemoveElement(Ui.Frags[i])
            UiRemoveElement(Ui.Deaths[i])
            UiRemoveElement(Ui.Ratio[i])
            UiRemoveElement(Ui.Fpm[i])
            UiRemoveElement(Ui.Hammer[i])
            UiRemoveElement(Ui.Gun[i])
            UiRemoveElement(Ui.Shotgun[i])
            UiRemoveElement(Ui.Grenade[i])
            UiRemoveElement(Ui.Rifle[i])
            UiRemoveElement(Ui.Katana[i])
            UiRemoveElement(Ui.Flags[i])
            UiRemoveElement(Ui.ID[i])
        end
    end
end
