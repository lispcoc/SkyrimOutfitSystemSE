Scriptname Ammo extends Form Hidden

; SKSE 64 additions built 2020-07-29 17:24:48.495000 UTC

; Returns whether this ammo is a bolt
bool Function IsBolt() native

; Returns the projectile associated with this ammo
Projectile Function GetProjectile() native

; Returns the base damage of this ammo
float Function GetDamage() native
