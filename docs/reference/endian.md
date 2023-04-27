
# Module: ninty.endian

<code>**def swap_array**(data: bytes, size: int) -> bytes</code><br>
<span class="docs">Swaps an array of elements with the given `size`.</span>

<code>**def swap_array**(data: bytes, size: int, offset: int, stride: int) -> bytes</code><br>
<span class="docs">Swaps elements of the given `size` at the given `offset` in a structured array with the given `stride`.</span>

<code>**def swap_array**(data: bytes, size: int, offset: int, count: int, stride: int) -> bytes</code><br>
<span class="docs">Swaps `count` elements of the given `size` at the given `offset` in a structured array with the given `stride`.</span>
