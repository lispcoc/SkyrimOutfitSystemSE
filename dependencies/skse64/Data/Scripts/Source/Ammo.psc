Scriptname Ammo extends Form Hidden

; SKSE64 additions built 2019-03-14 18:25:19.543000 UTC

; Returns whether this ammo is a bolt
bool Function IsBolt() native

; Returns the projectile associated with this ammo
Projectile Function GetProjectile() native

; Returns the base damage of this ammo
float Function GetDamage() native
