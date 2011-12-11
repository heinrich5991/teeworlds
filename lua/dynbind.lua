pi = 3.141592654

SetScriptTitle("Dynamic Camera Bind")
SetScriptInfo("(c) by MAP94 and Mosii")
SetScriptUseSettingPage(0)

--Print("DynCam", "(c) 2011 by MAP94 and Mosii")

GlobalCameraSwitch = 0
function SwitchCamera(s)
    GlobalCameraSwitch = s or GlobalCameraSwitch
    if GlobalCameraSwitch == 0 then
        SetConfigValue("MouseDeadzone", 0)
        SetConfigValue("MouseFollowfactor", 0)
        SetConfigValue("MouseMaxDistance", 400)
        GlobalCameraSwitch = 1
    else
        SetConfigValue("MouseDeadzone", 300)
        SetConfigValue("MouseFollowfactor", 60)
        SetConfigValue("MouseMaxDistance", 1000)
        GlobalCameraSwitch = 0
    end
end

--bind <key> +lua SwitchCamera
