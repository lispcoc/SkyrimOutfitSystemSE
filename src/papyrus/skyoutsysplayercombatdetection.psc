Scriptname SkyOutSysPlayerCombatDetection Extends ActiveMagicEffect

Event OnEffectStart(Actor akTarget, Actor akCaster)
    SkyrimOutfitSystemNativeFuncs.NotifyCombatStateChanged(akTarget)
Endevent
Event OnEffectFinish(Actor akTarget, Actor akCaster)
    SkyrimOutfitSystemNativeFuncs.NotifyCombatStateChanged(akTarget)
EndEvent
