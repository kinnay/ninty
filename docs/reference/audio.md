
# Module: ninty.audio

<code>**def interleave**(channels: list[bytes]) -> bytes</code>
<span class="docs">Interleaves the given PCM-16 channels. Every channel must contain the same number of samples.</span>

<code>**def deinterleave**(data: bytes, channels: int) -> list[bytes]</code>
<span class="docs">Deinterleaves the given PCM-16 samples into the given number of channels.</span>

<code>**def decode_pcm8**(data: bytes) -> bytes</code>
<span class="docs">Decodes PCM-8 samples to PCM-16.</span>

<code>**def encode_pcm8**(data: bytes) -> bytes</code>
<span class="docs">Encodes PCM-16 samples as PCM-8. This is a lossy compression algorithm.</span>

<code>**def decode_adpcm**(data: bytes, samples: int, coefs: list[int]) -> bytes</code>
<span class="docs">Decompresses the given number of ADPCM samples to PCM-16 using the given ADPCM coefficients.</span>

<code>**def encode_adpcm**(data: bytes) -> tuple[bytes, list[int]]</code>
<span class="docs">Compresses the given PCM-16 samples as ADPCM. The returned tuple contains the compressed samples and ADPCM coefficients. This is a lossy compression algorithm.

<code>**def get_adpcm_context**(data: bytes, samples: int, coefs: list[int]) -> tuple[int, int, int]</code>
<span class="docs">Determines the state of the ADPCM algorithm at the given sample. The returned tuple contains the current pred/scale byte, and the previous two samples.</span>
