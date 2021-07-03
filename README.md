# Nintendo extensions
C++ extension with functions for which python is too slow

To install this, run `pip3 install ninty`.

## Usage
```python
from ninty import audio, endian, gx2, lzss, yaz0

audio.interleave(channels: list[bytes]) -> bytes
audio.decode_pcm8(data: bytes, samples: int) -> bytes
audio.decode_adpcm(data: bytes, samples: int, coefs: list[int], header: int, hist1: int, hist2: int) -> bytes

endian.swap_array(data: bytes, size: int) -> bytes
endian.swap_array(data: bytes, size: int, offset: int, stride: int) -> bytes
endian.swap_array(data: bytes, size: int, offset: int, count: int, stride: int) -> bytes

gx2.deswizzle(data: bytes, width: int, height: int, format: int, tilemode: int, swizzle: int) -> bytes
gx2.decode(data: bytes, width: int, height: int, format: int) -> bytes

lzss.decompress(data: bytes, decompressed_size: int) -> bytes

yaz0.decompress(data: bytes, decompressed_size: int) -> bytes
yaz0.compress(data: bytes, window_size: int) -> bytes
```
