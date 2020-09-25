Scriptname TextureSet extends Form Hidden


; SKSE64 additions built 2019-03-14 18:25:19.543000 UTC

; Returns the number of texture paths
int Function GetNumTexturePaths() native

; Returns the path of the texture
string Function GetNthTexturePath(int n) native

; Sets the path of the texture
Function SetNthTexturePath(int n, string texturePath) native