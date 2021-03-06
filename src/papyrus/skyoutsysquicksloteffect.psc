Scriptname SkyOutSysQuickslotEffect extends activemagiceffect

Event OnEffectStart(Actor akCaster, Actor akTarget)
   String sCurrentOutfit = SkyrimOutfitSystemNativeFuncs.GetSelectedOutfit()
   String[] sLMenuItems = SkyrimOutfitSystemNativeFuncs.ListOutfits(favoritesOnly = true)
   sLMenuItems = SkyrimOutfitSystemNativeFuncs.NaturalSort_ASCII(sLMenuItems)
   UIListMenu menu = UIExtensions.GetMenu("UIListMenu") as UIListMenu
   Int iIndex = 0
   If sCurrentOutfit == ""
      sCurrentOutfit = "[DISMISS]NO OUTFIT"
   Else
      sCurrentOutfit = "[DISMISS]" + sCurrentOutfit
   Endif
   menu.AddEntryItem(sCurrentOutfit)
   menu.AddEntryItem("[NO OUTFIT]")
   While iIndex < sLMenuItems.Length
      menu.AddEntryItem("[SELECT]" + sLMenuItems[iIndex])
      iIndex = iIndex + 1
   EndWhile
   iIndex = 0
   While iIndex < sLMenuItems.Length
      menu.AddEntryItem("[OVERWRITE]" + sLMenuItems[iIndex])
      iIndex = iIndex + 1
   EndWhile
   UIExtensions.OpenMenu("UIListMenu")
   String result = menu.GetResultString()
   Debug.Trace("User selected outfit: " + result)
   If result == "" ; Cover the case where the user backs out of the menu
      result = "[DISMISS]"
   Endif
   If result == "[NO OUTFIT]"
      result = ""
      SkyrimOutfitSystemNativeFuncs.SetSelectedOutfit(result)
      SkyrimOutfitSystemNativeFuncs.RefreshArmorFor(Game.GetPlayer())
   Endif
   If StringUtil.Substring(result, 0, 8) == "[SELECT]"
      result = StringUtil.Substring(result, 9, 0)
      SkyrimOutfitSystemNativeFuncs.SetSelectedOutfit(result)
      ; Update the autoswitch slot if
      ; 1) autoswitching is enabled,
      ; 2) the current location has an outfit assigned already, and
      ; 3) if we have an outfit selected in this menu
      Int playerLocationType = SkyrimOutfitSystemNativeFuncs.IdentifyLocationType(Game.GetPlayer().GetCurrentLocation(), Weather.GetCurrentWeather())
      If SkyrimOutfitSystemNativeFuncs.GetLocationBasedAutoSwitchEnabled() && SkyrimOutfitSystemNativeFuncs.GetLocationOutfit(playerLocationType) != "" && result != ""
         SkyrimOutfitSystemNativeFuncs.SetLocationOutfit(playerLocationType, result)
         Debug.Notification("This outfit will be remembered for this location type.")
      EndIf
      SkyrimOutfitSystemNativeFuncs.RefreshArmorFor(Game.GetPlayer())
   Endif
   If StringUtil.Substring(result, 0, 11) == "[OVERWRITE]"
      result = StringUtil.Substring(result, 12, 0)
      Armor[] kList = SkyrimOutfitSystemNativeFuncs.GetWornItems(Game.GetPlayer())
      SkyrimOutfitSystemNativeFuncs.OverwriteOutfit(result, kList)
      SkyrimOutfitSystemNativeFuncs.RefreshArmorFor(Game.GetPlayer())
   Endif
EndEvent
