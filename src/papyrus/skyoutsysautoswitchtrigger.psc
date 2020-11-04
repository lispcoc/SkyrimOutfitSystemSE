Scriptname SkyOutSysAutoSwitchTrigger extends ReferenceAlias

Event OnLocationChange(Location akOldLoc, Location akNewLoc)
    Debug.Notification("SOS: Running OnLocationChange")
    Debug.Trace("SOS: Running OnLocationChange")
    SkyrimOutfitSystemNativeFuncs.SetOutfitUsingLocation(akNewLoc)
endEvent
