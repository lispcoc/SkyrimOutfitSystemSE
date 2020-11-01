Scriptname SkyOutSysQuickslotEffect extends activemagiceffect

Event OnEffectStart(Actor akCaster, Actor akTarget)
   String[] sLMenuItems = SkyrimOutfitSystemNativeFuncs.ListOutfits(favoritesOnly = true)
   sLMenuItems = SkyrimOutfitSystemNativeFuncs.NaturalSort_ASCII(sLMenuItems)
   UIListMenu menu = UIExtensions.GetMenu("UIListMenu") as UIListMenu
   Int iIndex = 0
   menu.AddEntryItem("[DISMISS]")
   menu.AddEntryItem("[NO OUTFIT]")
   While iIndex < sLMenuItems.Length
      menu.AddEntryItem(sLMenuItems[iIndex])
      iIndex = iIndex + 1
   EndWhile
   UIExtensions.OpenMenu("UIListMenu")
   String result = menu.GetResultString()
   Debug.Trace("User selected outfit: " + result)
   If result == "[NO OUTFIT]"
      result = ""
   Endif
   If result != "[DISMISS]"
      SkyrimOutfitSystemNativeFuncs.SetSelectedOutfit(result)
      SkyrimOutfitSystemNativeFuncs.RefreshArmorFor(Game.GetPlayer())
   Endif
EndEvent
