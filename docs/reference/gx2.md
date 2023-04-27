
# Module: ninty.gx2

<code>**class [Surface](#surface)**</code><br>
<span class="docs">Represents a `GX2Surface`.</span>

<code>**SUPPORTED_SURFACE_FORMATS**: list</code><br>
<span class="docs">The list of surface formats supported by `decode()`.</span>

<code>**def deswizzle**(data: bytes, width: int, height: int, format: int, tilemode: int, swizzle: int) -> bytes</code><br>
<span class="docs">Deswizzles a 2D texture and its mipmaps with the given parameters. All texture formats and tile modes are supported.</span>

<code>**def swizzle**(data: bytes, width: int, height: int, format: int, tilemode: int, swizzle: int) -> bytes</code><br>
<span class="docs">Swizzles a 2D texture and its mipmaps with the given parameters. All texture formats and tile modes are supported.</span>

<code>**def decode**(data: bytes, width: int, height: int, format: int) -> bytes</code><br>
<span class="docs">Decodes a 2D texture and its mipmaps to RGBA. Only a limited number of formats are supported.</span>

## Surface
<code>**dim**: int = GX2_SURFACE_DIM_TEXTURE_2D</code><br>
<code>**width**: int = 0</code><br>
<code>**height**: int = 0</code><br>
<code>**depth**: int = 0</code><br>
<code>**mip_levels**: int = 0</code><br>
<code>**format**: int = GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8</code><br>
<code>**aa**: int = GX2_AA_MODE_1X</code><br>
<code>**use**: int = GX2_SURFACE_USE_TEXTURE</code><br>
<code>**image_size**: int = 0</code><br>
<code>**image**: bytes = b""</code><br>
<code>**mipmap_size**: int = 0</code><br>
<code>**mipmaps**: bytes = b""</code><br>
<code>**tile_mode**: int = GX2_TILE_MODE_LINEAR_SPECIAL</code><br>
<code>**swizzle**: int = 0</code><br>
<code>**alignment**: int = 0</code><br>
<code>**pitch**: int = 0</code><br>
<code>**mip_level_offset**: list[int] = [0] * 13</code>

<code>**def \_\_init__**()</code><br>
<span class="docs">Creates a new [Surface](#surface) object.</span>

<code>**def calc_size_and_alignment**() -> None</code><br>
<span class="docs">This is an implementation of `GX2CalcSurfaceSizeAndAlignment`.<br><br>The following fields are updated: `alignment`, `pitch`, `swizzle`, `image_size`, `mipmap_size`, `mip_levels` and `mip_level_offset`.<br><br>If `tilemode` is set to `GX2_TILE_MODE_DEFAULT` it is updated as well.

<code>**def deswizzle**() -> None</code><br>
<span class="docs">Deswizzles the surface.</span>

<code>**def swizzle**(tilemode: int, swizzle: int) -> None</code><br>
<span class="docs">Swizzles the surface.</span>

<code>**def decode**() -> None</code><br>
<span class="docs">Decodes the surface to RGBA. Only deswizzled surfaces can be decoded.</span>
