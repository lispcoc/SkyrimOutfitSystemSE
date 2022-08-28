Scriptname SkyOutSysAutoSwitchTrigger extends ReferenceAlias

Auto State Listening
Event OnLocationChange(Location akOldLoc, Location akNewLoc)
    ; Debug.Trace("SOS: Running OnLocationChange")
    GoToState("Waiting")
    Utility.Wait(10.0)
    SkyrimOutfitSystemNativeFuncs.SetOutfitUsingLocation(Game.GetPlayer(), Game.GetPlayer().GetCurrentLocation(), Weather.GetCurrentWeather())
    SkyrimOutfitSystemNativeFuncs.RefreshArmorFor(Game.GetPlayer())
    GoToState("Listening")
endEvent
EndState

State Waiting
Event OnLocationChange(Location akOldLoc, Location akNewLoc)
    ; Ignore changes while we're waiting
endEvent
EndState