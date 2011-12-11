Config = table
--Std config
Config.Name1 = "idm"
Config.R1 = 0
Config.G1 = 0
Config.B1 = 99
Config.Name2 = "ictf"
Config.R2 = 0
Config.G2 = 0
Config.B2 = 99
Config.Name3 = "itdm"
Config.R3 = 0
Config.G3 = 0
Config.B3 = 99
Config.Name4 = "race"
Config.R4 = 99
Config.G4 = 0
Config.B4 = 77
Config.Name5 = "ddrace"
Config.R5 = 99
Config.G5 = 0
Config.B5 = 44
Config.Name6 = "wictf"
Config.R6 = 44
Config.G6 = 66
Config.B6 = 99
Config.Name7 = "openfng"
Config.R7 = 0
Config.G7 = 99
Config.B7 = 66
Config.Name8 = "dm"
Config.R8 = 99
Config.G8 = 0
Config.B8 = 0
Config.Name9 = "tdm"
Config.R9 = 99
Config.G9 = 0
Config.B9 = 0
Config.Name10 = "ctf"
Config.R10 = 99
Config.G10 = 0
Config.B10 = 0
Config.Name11 = ""
Config.R11 = 0
Config.G11 = 0
Config.B11 = 0
Config.Name12 = ""
Config.R12 = 0
Config.G12 = 0
Config.B12 = 0

Include ("lua/colored_gametypes.config")

pi = 3.141592654

SetScriptTitle("Colored Gametypes")
SetScriptInfo("(c) by Mosii")
SetScriptUseSettingPage(1)

AddEventListener("OnServerBrowserGameTypeRender", "SetColor")

function SetColor()
    Name = GetMenuBrowserGameTypeName()
    Name = Name:lower()
    if (Name == Config.Name1) then
        SetMenuBrowserGameTypeColor(Config.R1 / 99, Config.G1 / 99, Config.B1 / 99, 1)
    end
    if (Name == Config.Name2) then
        SetMenuBrowserGameTypeColor(Config.R2 / 99, Config.G2 / 99, Config.B2 / 99, 1)
    end
    if (Name == Config.Name3) then
        SetMenuBrowserGameTypeColor(Config.R3 / 99, Config.G3 / 99, Config.B3 / 99, 1)
    end
    if (Name == Config.Name4) then
        SetMenuBrowserGameTypeColor(Config.R4 / 99, Config.G4 / 99, Config.B4 / 99, 1)
    end
    if (Name == Config.Name5) then
        SetMenuBrowserGameTypeColor(Config.R5 / 99, Config.G5 / 99, Config.B5 / 99, 1)
    end
    if (Name == Config.Name6) then
        SetMenuBrowserGameTypeColor(Config.R6 / 99, Config.G6 / 99, Config.B6 / 99, 1)
    end
    if (Name == Config.Name7) then
        SetMenuBrowserGameTypeColor(Config.R7 / 99, Config.G7 / 99, Config.B7 / 99, 1)
    end
    if (Name == Config.Name8) then
        SetMenuBrowserGameTypeColor(Config.R8 / 99, Config.G8 / 99, Config.B8 / 99, 1)
    end
    if (Name == Config.Name9) then
        SetMenuBrowserGameTypeColor(Config.R9 / 99, Config.G9 / 99, Config.B9 / 99, 1)
    end
    if (Name == Config.Name10) then
        SetMenuBrowserGameTypeColor(Config.R10 / 99, Config.G10 / 99, Config.B10 / 99, 1)
    end
    if (Name == Config.Name11) then
        SetMenuBrowserGameTypeColor(Config.R11 / 99, Config.G1 / 99, Config.B11 / 99, 1)
    end
    if (Name == Config.Name12) then
        SetMenuBrowserGameTypeColor(Config.R12 / 99, Config.G12 / 99, Config.B12 / 99, 1)
    end
end

-- Config
Ui = table
Ui.BaseRect = nil

Ui.G1GameTypeBox = nil
Ui.G1RBox = nil
Ui.G1GBox = nil
Ui.G1BBox = nil
Ui.G1Label = nil
Ui.G1Preview = nil

Ui.G2GameTypeBox = nil
Ui.G2RBox = nil
Ui.G2GBox = nil
Ui.G2BBox = nil
Ui.G2Label = nil
Ui.G2Preview = nil

Ui.G3GameTypeBox = nil
Ui.G3RBox = nil
Ui.G3GBox = nil
Ui.G3BBox = nil
Ui.G3Label = nil
Ui.G3Preview = nil

Ui.G4GameTypeBox = nil
Ui.G4RBox = nil
Ui.G4GBox = nil
Ui.G4BBox = nil
Ui.G4Label = nil
Ui.G4Preview = nil

Ui.G5GameTypeBox = nil
Ui.G5RBox = nil
Ui.G5GBox = nil
Ui.G5BBox = nil
Ui.G5Label = nil
Ui.G5Preview = nil

Ui.G6GameTypeBox = nil
Ui.G6RBox = nil
Ui.G6GBox = nil
Ui.G6BBox = nil
Ui.G6Label = nil
Ui.G6Preview = nil

Ui.G7GameTypeBox = nil
Ui.G7RBox = nil
Ui.G7GBox = nil
Ui.G7BBox = nil
Ui.G7Label = nil
Ui.G7Preview = nil

Ui.G8GameTypeBox = nil
Ui.G8RBox = nil
Ui.G8GBox = nil
Ui.G8BBox = nil
Ui.G8Label = nil
Ui.G8Preview = nil

Ui.G9GameTypeBox = nil
Ui.G9RBox = nil
Ui.G9GBox = nil
Ui.G9BBox = nil
Ui.G9Label = nil
Ui.G9Preview = nil

Ui.G10GameTypeBox = nil
Ui.G10RBox = nil
Ui.G10GBox = nil
Ui.G10BBox = nil
Ui.G10Label = nil
Ui.G10Preview = nil

Ui.G11GameTypeBox = nil
Ui.G11RBox = nil
Ui.G11GBox = nil
Ui.G11BBox = nil
Ui.G11Label = nil
Ui.G11Preview = nil

Ui.G12GameTypeBox = nil
Ui.G12RBox = nil
Ui.G12GBox = nil
Ui.G12BBox = nil
Ui.G12Label = nil
Ui.G12Preview = nil

Ui.Activ = false;
function ConfigOpen(x, y, w, h)
    Ui.Activ = true

    Ui.BaseRect = UiDoRect(x, y, w, h, 0, 15, 15, 0, 0, 0, 0.5)

    Ui.G1Label = UiDoLabel(x + 10, y + 10, 100, 20, 0, "Gametype", 20, -1)
    Ui.G1GameTypeBox = UiDoEditBox(x + 110, y + 10, 50, 20, 0, Config.Name1)
    Ui.G1RBox = UiDoEditBox(x + 180, y + 10, 50, 20, 0, Config.R1)
    Ui.G1GBox = UiDoEditBox(x + 240, y + 10, 50, 20, 0, Config.G1)
    Ui.G1BBox = UiDoEditBox(x + 300, y + 10, 50, 20, 0, Config.B1)
    Ui.G1Preview = UiDoRect(x + 360, y + 10, 20, 20, 15, 5, 0, 0, 0, 1)

    y = y + 30

    Ui.G2Label = UiDoLabel(x + 10, y + 10, 100, 20, 0, "Gametype", 20, -1)
    Ui.G2GameTypeBox = UiDoEditBox(x + 110, y + 10, 50, 20, 0, Config.Name2)
    Ui.G2RBox = UiDoEditBox(x + 180, y + 10, 50, 20, 0, Config.R2)
    Ui.G2GBox = UiDoEditBox(x + 240, y + 10, 50, 20, 0, Config.G2)
    Ui.G2BBox = UiDoEditBox(x + 300, y + 10, 50, 20, 0, Config.B2)
    Ui.G2Preview = UiDoRect(x + 360, y + 10, 20, 20, 15, 5, 0, 0, 0, 1)

    y = y + 30

    Ui.G3Label = UiDoLabel(x + 10, y + 10, 100, 20, 0, "Gametype", 20, -1)
    Ui.G3GameTypeBox = UiDoEditBox(x + 110, y + 10, 50, 20, 0, Config.Name3)
    Ui.G3RBox = UiDoEditBox(x + 180, y + 10, 50, 20, 0, Config.R3)
    Ui.G3GBox = UiDoEditBox(x + 240, y + 10, 50, 20, 0, Config.G3)
    Ui.G3BBox = UiDoEditBox(x + 300, y + 10, 50, 20, 0, Config.B3)
    Ui.G3Preview = UiDoRect(x + 360, y + 10, 20, 20, 15, 5, 0, 0, 0, 1)

    y = y + 30

    Ui.G4Label = UiDoLabel(x + 10, y + 10, 100, 20, 0, "Gametype", 20, -1)
    Ui.G4GameTypeBox = UiDoEditBox(x + 110, y + 10, 50, 20, 0, Config.Name4)
    Ui.G4RBox = UiDoEditBox(x + 180, y + 10, 50, 20, 0, Config.R4)
    Ui.G4GBox = UiDoEditBox(x + 240, y + 10, 50, 20, 0, Config.G4)
    Ui.G4BBox = UiDoEditBox(x + 300, y + 10, 50, 20, 0, Config.B4)
    Ui.G4Preview = UiDoRect(x + 360, y + 10, 20, 20, 15, 5, 0, 0, 0, 1)

    y = y + 30

    Ui.G5Label = UiDoLabel(x + 10, y + 10, 100, 20, 0, "Gametype", 20, -1)
    Ui.G5GameTypeBox = UiDoEditBox(x + 110, y + 10, 50, 20, 0, Config.Name5)
    Ui.G5RBox = UiDoEditBox(x + 180, y + 10, 50, 20, 0, Config.R5)
    Ui.G5GBox = UiDoEditBox(x + 240, y + 10, 50, 20, 0, Config.G5)
    Ui.G5BBox = UiDoEditBox(x + 300, y + 10, 50, 20, 0, Config.B5)
    Ui.G5Preview = UiDoRect(x + 360, y + 10, 20, 20, 15, 5, 0, 0, 0, 1)

    y = y + 30

    Ui.G6Label = UiDoLabel(x + 10, y + 10, 100, 20, 0, "Gametype", 20, -1)
    Ui.G6GameTypeBox = UiDoEditBox(x + 110, y + 10, 50, 20, 0, Config.Name6)
    Ui.G6RBox = UiDoEditBox(x + 180, y + 10, 50, 20, 0, Config.R6)
    Ui.G6GBox = UiDoEditBox(x + 240, y + 10, 50, 20, 0, Config.G6)
    Ui.G6BBox = UiDoEditBox(x + 300, y + 10, 50, 20, 0, Config.B6)
    Ui.G6Preview = UiDoRect(x + 360, y + 10, 20, 20, 15, 5, 0, 0, 0, 1)

    y = y + 30

    Ui.G7Label = UiDoLabel(x + 10, y + 10, 100, 20, 0, "Gametype", 20, -1)
    Ui.G7GameTypeBox = UiDoEditBox(x + 110, y + 10, 50, 20, 0, Config.Name7)
    Ui.G7RBox = UiDoEditBox(x + 180, y + 10, 50, 20, 0, Config.R7)
    Ui.G7GBox = UiDoEditBox(x + 240, y + 10, 50, 20, 0, Config.G7)
    Ui.G7BBox = UiDoEditBox(x + 300, y + 10, 50, 20, 0, Config.B7)
    Ui.G7Preview = UiDoRect(x + 360, y + 10, 20, 20, 15, 5, 0, 0, 0, 1)

    y = y + 30

    Ui.G8Label = UiDoLabel(x + 10, y + 10, 100, 20, 0, "Gametype", 20, -1)
    Ui.G8GameTypeBox = UiDoEditBox(x + 110, y + 10, 50, 20, 0, Config.Name8)
    Ui.G8RBox = UiDoEditBox(x + 180, y + 10, 50, 20, 0, Config.R8)
    Ui.G8GBox = UiDoEditBox(x + 240, y + 10, 50, 20, 0, Config.G8)
    Ui.G8BBox = UiDoEditBox(x + 300, y + 10, 50, 20, 0, Config.B8)
    Ui.G8Preview = UiDoRect(x + 360, y + 10, 20, 20, 15, 5, 0, 0, 0, 1)

    y = y + 30

    Ui.G9Label = UiDoLabel(x + 10, y + 10, 100, 20, 0, "Gametype", 20, -1)
    Ui.G9GameTypeBox = UiDoEditBox(x + 110, y + 10, 50, 20, 0, Config.Name9)
    Ui.G9RBox = UiDoEditBox(x + 180, y + 10, 50, 20, 0, Config.R9)
    Ui.G9GBox = UiDoEditBox(x + 240, y + 10, 50, 20, 0, Config.G9)
    Ui.G9BBox = UiDoEditBox(x + 300, y + 10, 50, 20, 0, Config.B9)
    Ui.G9Preview = UiDoRect(x + 360, y + 10, 20, 20, 15, 5, 0, 0, 0, 1)

    y = y + 30

    Ui.G10Label = UiDoLabel(x + 10, y + 10, 100, 20, 0, "Gametype", 20, -1)
    Ui.G10GameTypeBox = UiDoEditBox(x + 110, y + 10, 50, 20, 0, Config.Name10)
    Ui.G10RBox = UiDoEditBox(x + 180, y + 10, 50, 20, 0, Config.R10)
    Ui.G10GBox = UiDoEditBox(x + 240, y + 10, 50, 20, 0, Config.G10)
    Ui.G10BBox = UiDoEditBox(x + 300, y + 10, 50, 20, 0, Config.B10)
    Ui.G10Preview = UiDoRect(x + 360, y + 10, 20, 20, 15, 5, 0, 0, 0, 1)

    y = y + 30

    Ui.G11Label = UiDoLabel(x + 10, y + 10, 100, 20, 0, "Gametype", 20, -1)
    Ui.G11GameTypeBox = UiDoEditBox(x + 110, y + 10, 50, 20, 0, Config.Name11)
    Ui.G11RBox = UiDoEditBox(x + 180, y + 10, 50, 20, 0, Config.R11)
    Ui.G11GBox = UiDoEditBox(x + 240, y + 10, 50, 20, 0, Config.G11)
    Ui.G11BBox = UiDoEditBox(x + 300, y + 10, 50, 20, 0, Config.B11)
    Ui.G11Preview = UiDoRect(x + 360, y + 10, 20, 20, 15, 5, 0, 0, 0, 1)

    y = y + 30

    Ui.G12Label = UiDoLabel(x + 10, y + 10, 100, 20, 0, "Gametype", 20, -1)
    Ui.G12GameTypeBox = UiDoEditBox(x + 110, y + 10, 50, 20, 0, Config.Name12)
    Ui.G12RBox = UiDoEditBox(x + 180, y + 10, 50, 20, 0, Config.R12)
    Ui.G12GBox = UiDoEditBox(x + 240, y + 10, 50, 20, 0, Config.G12)
    Ui.G12BBox = UiDoEditBox(x + 300, y + 10, 50, 20, 0, Config.B12)
    Ui.G12Preview = UiDoRect(x + 360, y + 10, 20, 20, 15, 5, 0, 0, 0, 1)

end

iTick = 0
function Tick(Time, ServerTick)
    iTick = iTick + 1
    Time = Time or 0
    ServerTick = ServerTick or 0

    if (Ui.Activ) then
        UiSetColor(Ui.G1Preview, UiGetText(Ui.G1RBox) / 99, UiGetText(Ui.G1GBox) / 99, UiGetText(Ui.G1BBox) / 99, 1)
        UiSetColor(Ui.G2Preview, UiGetText(Ui.G2RBox) / 99, UiGetText(Ui.G2GBox) / 99, UiGetText(Ui.G2BBox) / 99, 1)
        UiSetColor(Ui.G3Preview, UiGetText(Ui.G3RBox) / 99, UiGetText(Ui.G3GBox) / 99, UiGetText(Ui.G3BBox) / 99, 1)
        UiSetColor(Ui.G4Preview, UiGetText(Ui.G4RBox) / 99, UiGetText(Ui.G4GBox) / 99, UiGetText(Ui.G4BBox) / 99, 1)
        UiSetColor(Ui.G5Preview, UiGetText(Ui.G5RBox) / 99, UiGetText(Ui.G5GBox) / 99, UiGetText(Ui.G5BBox) / 99, 1)
        UiSetColor(Ui.G6Preview, UiGetText(Ui.G6RBox) / 99, UiGetText(Ui.G6GBox) / 99, UiGetText(Ui.G6BBox) / 99, 1)
        UiSetColor(Ui.G7Preview, UiGetText(Ui.G7RBox) / 99, UiGetText(Ui.G7GBox) / 99, UiGetText(Ui.G7BBox) / 99, 1)
        UiSetColor(Ui.G8Preview, UiGetText(Ui.G8RBox) / 99, UiGetText(Ui.G8GBox) / 99, UiGetText(Ui.G8BBox) / 99, 1)
        UiSetColor(Ui.G9Preview, UiGetText(Ui.G9RBox) / 99, UiGetText(Ui.G9GBox) / 99, UiGetText(Ui.G9BBox) / 99, 1)
        UiSetColor(Ui.G10Preview, UiGetText(Ui.G10RBox) / 99, UiGetText(Ui.G10GBox) / 99, UiGetText(Ui.G10BBox) / 99, 1)
        UiSetColor(Ui.G11Preview, UiGetText(Ui.G11RBox) / 99, UiGetText(Ui.G11GBox) / 99, UiGetText(Ui.G11BBox) / 99, 1)
        UiSetColor(Ui.G12Preview, UiGetText(Ui.G12RBox) / 99, UiGetText(Ui.G12GBox) / 99, UiGetText(Ui.G12BBox) / 99, 1)
    end
end


function ConfigClose(x, y, w, h)
    Ui.Activ = false

    configout = io.open("lua/colored_gametypes.config", "wb")
    configout:write("--Configfile for Colored Gametypes\n")
    configout:write("Config.Name1 = \"" .. UiGetText(Ui.G1GameTypeBox):lower() .. "\"\n")
    configout:write("Config.R1 = " .. UiGetText(Ui.G1RBox) .. "\n")
    configout:write("Config.G1 = " .. UiGetText(Ui.G1GBox) .. "\n")
    configout:write("Config.B1 = " .. UiGetText(Ui.G1BBox) .. "\n")

    configout:write("Config.Name2 = \"" .. UiGetText(Ui.G2GameTypeBox):lower() .. "\"\n")
    configout:write("Config.R2 = " .. UiGetText(Ui.G2RBox) .. "\n")
    configout:write("Config.G2 = " .. UiGetText(Ui.G2GBox) .. "\n")
    configout:write("Config.B2 = " .. UiGetText(Ui.G2BBox) .. "\n")

    configout:write("Config.Name3 = \"" .. UiGetText(Ui.G3GameTypeBox):lower() .. "\"\n")
    configout:write("Config.R3 = " .. UiGetText(Ui.G3RBox) .. "\n")
    configout:write("Config.G3 = " .. UiGetText(Ui.G3GBox) .. "\n")
    configout:write("Config.B3 = " .. UiGetText(Ui.G3BBox) .. "\n")

    configout:write("Config.Name4 = \"" .. UiGetText(Ui.G4GameTypeBox):lower() .. "\"\n")
    configout:write("Config.R4 = " .. UiGetText(Ui.G4RBox) .. "\n")
    configout:write("Config.G4 = " .. UiGetText(Ui.G4GBox) .. "\n")
    configout:write("Config.B4 = " .. UiGetText(Ui.G4BBox) .. "\n")

    configout:write("Config.Name5 = \"" .. UiGetText(Ui.G5GameTypeBox):lower() .. "\"\n")
    configout:write("Config.R5 = " .. UiGetText(Ui.G5RBox) .. "\n")
    configout:write("Config.G5 = " .. UiGetText(Ui.G5GBox) .. "\n")
    configout:write("Config.B5 = " .. UiGetText(Ui.G5BBox) .. "\n")

    configout:write("Config.Name6 = \"" .. UiGetText(Ui.G6GameTypeBox):lower() .. "\"\n")
    configout:write("Config.R6 = " .. UiGetText(Ui.G6RBox) .. "\n")
    configout:write("Config.G6 = " .. UiGetText(Ui.G6GBox) .. "\n")
    configout:write("Config.B6 = " .. UiGetText(Ui.G6BBox) .. "\n")

    configout:write("Config.Name7 = \"" .. UiGetText(Ui.G7GameTypeBox):lower() .. "\"\n")
    configout:write("Config.R7 = " .. UiGetText(Ui.G7RBox) .. "\n")
    configout:write("Config.G7 = " .. UiGetText(Ui.G7GBox) .. "\n")
    configout:write("Config.B7 = " .. UiGetText(Ui.G7BBox) .. "\n")

    configout:write("Config.Name8 = \"" .. UiGetText(Ui.G8GameTypeBox):lower() .. "\"\n")
    configout:write("Config.R8 = " .. UiGetText(Ui.G8RBox) .. "\n")
    configout:write("Config.G8 = " .. UiGetText(Ui.G8GBox) .. "\n")
    configout:write("Config.B8 = " .. UiGetText(Ui.G8BBox) .. "\n")

    configout:write("Config.Name9 = \"" .. UiGetText(Ui.G9GameTypeBox):lower() .. "\"\n")
    configout:write("Config.R9 = " .. UiGetText(Ui.G9RBox) .. "\n")
    configout:write("Config.G9 = " .. UiGetText(Ui.G9GBox) .. "\n")
    configout:write("Config.B9 = " .. UiGetText(Ui.G9BBox) .. "\n")

    configout:write("Config.Name10 = \"" .. UiGetText(Ui.G10GameTypeBox):lower() .. "\"\n")
    configout:write("Config.R10 = " .. UiGetText(Ui.G10RBox) .. "\n")
    configout:write("Config.G10 = " .. UiGetText(Ui.G10GBox) .. "\n")
    configout:write("Config.B10 = " .. UiGetText(Ui.G10BBox) .. "\n")

    configout:write("Config.Name11 = \"" .. UiGetText(Ui.G11GameTypeBox):lower() .. "\"\n")
    configout:write("Config.R11 = " .. UiGetText(Ui.G11RBox) .. "\n")
    configout:write("Config.G11 = " .. UiGetText(Ui.G11GBox) .. "\n")
    configout:write("Config.B11 = " .. UiGetText(Ui.G11BBox) .. "\n")

    configout:write("Config.Name12 = \"" .. UiGetText(Ui.G12GameTypeBox):lower() .. "\"\n")
    configout:write("Config.R12 = " .. UiGetText(Ui.G12RBox) .. "\n")
    configout:write("Config.G12 = " .. UiGetText(Ui.G12GBox) .. "\n")
    configout:write("Config.B12 = " .. UiGetText(Ui.G12BBox) .. "\n")
    configout:close()
    Include ("lua/colored_gametypes.config") -- refresh the configs
    UiRemoveElement(Ui.BaseRect)

    UiRemoveElement(Ui.G1GameTypeBox)
    UiRemoveElement(Ui.G1Label)
    UiRemoveElement(Ui.G1RBox)
    UiRemoveElement(Ui.G1GBox)
    UiRemoveElement(Ui.G1BBox)
    UiRemoveElement(Ui.G1Preview)

    UiRemoveElement(Ui.G2GameTypeBox)
    UiRemoveElement(Ui.G2Label)
    UiRemoveElement(Ui.G2RBox)
    UiRemoveElement(Ui.G2GBox)
    UiRemoveElement(Ui.G2BBox)
    UiRemoveElement(Ui.G2Preview)

    UiRemoveElement(Ui.G3GameTypeBox)
    UiRemoveElement(Ui.G3Label)
    UiRemoveElement(Ui.G3RBox)
    UiRemoveElement(Ui.G3GBox)
    UiRemoveElement(Ui.G3BBox)
    UiRemoveElement(Ui.G3Preview)

    UiRemoveElement(Ui.G4GameTypeBox)
    UiRemoveElement(Ui.G4Label)
    UiRemoveElement(Ui.G4RBox)
    UiRemoveElement(Ui.G4GBox)
    UiRemoveElement(Ui.G4BBox)
    UiRemoveElement(Ui.G4Preview)

    UiRemoveElement(Ui.G5GameTypeBox)
    UiRemoveElement(Ui.G5Label)
    UiRemoveElement(Ui.G5RBox)
    UiRemoveElement(Ui.G5GBox)
    UiRemoveElement(Ui.G5BBox)
    UiRemoveElement(Ui.G5Preview)

    UiRemoveElement(Ui.G6GameTypeBox)
    UiRemoveElement(Ui.G6Label)
    UiRemoveElement(Ui.G6RBox)
    UiRemoveElement(Ui.G6GBox)
    UiRemoveElement(Ui.G6BBox)
    UiRemoveElement(Ui.G6Preview)

    UiRemoveElement(Ui.G7GameTypeBox)
    UiRemoveElement(Ui.G7Label)
    UiRemoveElement(Ui.G7RBox)
    UiRemoveElement(Ui.G7GBox)
    UiRemoveElement(Ui.G7BBox)
    UiRemoveElement(Ui.G7Preview)

    UiRemoveElement(Ui.G8GameTypeBox)
    UiRemoveElement(Ui.G8Label)
    UiRemoveElement(Ui.G8RBox)
    UiRemoveElement(Ui.G8GBox)
    UiRemoveElement(Ui.G8BBox)
    UiRemoveElement(Ui.G8Preview)

    UiRemoveElement(Ui.G9GameTypeBox)
    UiRemoveElement(Ui.G9Label)
    UiRemoveElement(Ui.G9RBox)
    UiRemoveElement(Ui.G9GBox)
    UiRemoveElement(Ui.G9BBox)
    UiRemoveElement(Ui.G9Preview)

    UiRemoveElement(Ui.G10GameTypeBox)
    UiRemoveElement(Ui.G10Label)
    UiRemoveElement(Ui.G10RBox)
    UiRemoveElement(Ui.G10GBox)
    UiRemoveElement(Ui.G10BBox)
    UiRemoveElement(Ui.G10Preview)

    UiRemoveElement(Ui.G11GameTypeBox)
    UiRemoveElement(Ui.G11Label)
    UiRemoveElement(Ui.G11RBox)
    UiRemoveElement(Ui.G11GBox)
    UiRemoveElement(Ui.G11BBox)
    UiRemoveElement(Ui.G11Preview)

    UiRemoveElement(Ui.G12GameTypeBox)
    UiRemoveElement(Ui.G12Label)
    UiRemoveElement(Ui.G12RBox)
    UiRemoveElement(Ui.G12GBox)
    UiRemoveElement(Ui.G12BBox)
    UiRemoveElement(Ui.G12Preview)

end
