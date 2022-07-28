# MP3Stego

MP3Stego hides information in MP3 files during the compression process. The data is first compressed, encrypted and then hidden in the MP3 bit stream.

The hiding process takes place at the heart of the Layer III encoding process namely in the `inner_loop`. The inner loop quantizes the input data and increases the quantiser step size until the quantized data can be coded with the available number of bits. Another loop checks that the distortions introduced by the quantization do not exceed the threshold defined by the psycho acoustic model. The `part2_3_length` variable contains the number of `main_data` bits used for scalefactors and Huffman code data in the MP3 bit stream. We encode the bits as its parity by changing the end loop condition of the inner loop. Only randomly chosen `part2_3_length` values are modified; the selection is done using a pseudo random bit generator based on SHA-1.

The power of parity for information hiding has been discussed in [1]. MP3Stego is a practical example of it. 

## Compilation

MP3Stego has been compiled with Microsoft Visual Studio Community Edition. Open the `MP3Stego.sln` solution file located in the MP3Stego sub-folder.


## Usage example

`encode -E hidden_text.txt -P pass svega.wav svega_stego.mp3`

compresses svega.wav (mono, 44.1 kHz, 16bit encoded) and hides hidden_text.txt. The hidden text is encrypted using pass as a password. This produces the output called svega_stego.mp3. If no information was hidden, you would obtain this.

`decode -X -P pass svega_stego.mp3`

uncompresses svega_stego.mp3 into svega_stego.mp3.pcm and attempts to extract hidden information. The hidden message is decrypted, uncompressed and saved into svega_stego.mp3.txt.

## More information

Visit [https://www.petitcolas.net/steganography/mp3stego/](https://www.petitcolas.net/steganography/mp3stego/) for additional information.

## Reference

[1] Ross J. Anderson and Fabien A.P. Petitcolas. On The Limits of     Steganography. IEEE Journal of Selected Areas in Communications, 16(4):474-481, May 1998. Special Issue on Copyright & Privacy Protection. ISSN 0733-8716.

## History

| Release date      | Comment |
|-------------------|---------|
| 3 November 2018   | Correction of buffer overflow issue reported by tsls <tslsgogogo@gmail.com> |
| 13 June 2006      | Minor updates |
| 12 September 2002 | Bug fixed in StegoOpenEmbeddedText. |
| 19 Mars 2002      | Compression library has been updated to 1.1.4. |
| 20 December 1999  | MP3Stego now informs users if the data to be hidden is too big for the cover-sound. |
| 5 August 1998     | MP3Stego is advertised on both the steganography and watermarking mailing lists. |


## Important notice
This computer program is based on:

- 8hz-mp3 0.2b – 8Hz implementation of MP3 encoder;
- MP3 Decoder (dist10) of the ISO MPEG Audio Subgroup Software Simulation Group;
- ZLib 1.1.4 compression library by Jean-Loup Gailly’s ZLib;
- Eric’s Young implementation of 3DES;
- James J. Gillogly’s implementation of SHA-1;
- ISO/IEC 11172-3:1993, Information technology – Coding of moving pictures and associated audio for digital storage media at up to about 1,5 Mbit/s – Part 3: Audio, with the permission of ISO. Copies of this standards can be purchased from the British Standards Institution, 389 Chiswick High Road, GB-London W4 4AL, Telephone:+ 44 181 996 90 00, Telefax:+ 44 181 996 74 00 or from ISO, postal box 56, CH-1211 Geneva 20, Telephone +41 22 749 0111, Telefax +4122 734 1079. Copyright remains with ISO.