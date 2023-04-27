
# Module: ninty.yaz0

<code>**def compress**(data: bytes, window_size: int) -> bytes</code><br>
<span class="docs">Compresses data using the Yaz0 algorithm. The `window_size` should be between `0` (fastest compression) and `4096` (strongest compression).

<code>**def decompress**(data: bytes, decompressed_size: int) -> bytes</code><br>
<span class="docs">Decompresses data using the Yaz0 algorithm.</span>
