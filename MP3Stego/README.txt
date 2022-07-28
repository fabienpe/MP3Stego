MP3Stego
--------

Fabien A. P. Petitcolas, Cambridge
13 June 2006


When looking at the steganographic tools available on the Net, it occurred to
me that nothing had been done to hide information in MP3 files, that is sound
tracks compressed using the MPEG Audio Layer III format. There is a growing 
interest world-wide in MP3 files because they offer near-CD quality at
compression ratio of 11 to 1 (128 kilobits per second). This gives a very good 
opportunity for information hiding. 

MP3Stego will hide information in MP3 files during the compression process.
The data is first compressed, encrypted and then data hidden in the MP3 bit
stream. Although MP3Stego has been written with steganographic applications in
mind it might be used as a watermarking system for MP3 files. Any opponent can 
uncompress the bit stream and recompress it; this will delete the hidden
information -- actually this is the only attack we know yet -- but at the 
expense of severe quality loss.

The hiding process takes place at the heart of the Layer III encoding process 
namely in the inner_loop. The inner loop quantizes the input data and increases 
the quantizer step size until the quantized data can be coded with the available
number of bits. Another loop checks that the distortions introduced by the 
quantization do not exceed the threshold defined by the psycho acoustic model. 
The part2_3_length variable contains the number of main_data bits used for 
scalefactors and Huffman code data in the MP3 bit stream. We encode the bits 
as its parity by changing the end loop condition of the inner loop. Only 
randomly chosen part2_3_length values are modified; the selection is done 
using a pseudo random bit generator based on SHA-1. 

We have discussed earlier the power of parity for information hiding [1]. 
MP3Stego is a practical example of it. There is still space for improvement 
but I thought that some people might be interested to have a look at it. 


Compilation
-----------

The full C code and binaries are available from:

  <http://www.petitcolas.net/fabien/steganography/mp3stego/>

and compilation can easily be done by opening the MP3Stego.sln solution with
free Microsoft Studio Express available from:

  <http://msdn.microsoft.com/vstudio/express/visualc/>


Usage exemple
-------------

  encode -E data.txt -P pass sound.wav sound.mp3 
    compresses sound.wav and hides data.txt. This produces the output called 
    sound.mp3. The text in data.txt is encrypted using "pass".

  decode -X -P pass sound.mp3 
    uncompresses sound.mp3 into sound.mp3.pcm and attempts to extract hidden 
    information. The hidden message is decrypted, uncompressed and saved into 
    sound.mp3.txt. 


Feedback
--------

Don't forget to let me know your code updates, suggestions or comments to:

  fabien22@petitcolas.net 


Important notice
----------------

This computer program is based on:

- 8hz-mp3 0.2b -- 8Hz implementation of MP3 encoder; 
- MP3 Decoder (dist10) of the ISO MPEG Audio Subgroup Software Simulation Group; 
- ZLib 1.1.4 compression library by Jean-Loup Gailly’s ZLib;
- Eric’s Young implementation of 3DES; 
- James J. Gillogly’s implementation of SHA-1; 
- ISO/IEC 11172-3:1993, Information technology -- Coding of moving pictures 
  and associated audio for digital storage media at up to about 1,5 Mbit/s -- 
  Part 3: Audio, with the permission of ISO. Copies of this standards can be 
  purchased from the British Standards Institution, 389 Chiswick High Road, 
  GB-London W4 4AL, Telephone:+ 44 181 996 90 00, Telefax:+ 44 181 996 74 00 
  or from ISO, postal box 56, CH-1211 Geneva 20, Telephone +41 22 749 0111, 
  Telefax +4122 734 1079. Copyright remains with ISO. 


Reference
---------

[1] Ross J. Anderson and Fabien A.P. Petitcolas. On The Limits of 
    Steganography. IEEE Journal of Selected Areas in Communications, 
    16(4):474-481, May 1998. Special Issue on Copyright & Privacy Protection. 
    ISSN 0733-8716. 


History
-------

13 June 2006 -      Minor updates

12 September 2002 - Bug fixed in StegoOpenEmbeddedText

19 Mars 2002 -      Compression library has been updated to 1.1.4.

20 December 1999 -  MP3Stego now informs users if the data to be hidden is
                    too big for the cover-sound.

5 August 1998 -     MP3Stego is advertised on both the steganography and
                    watermarking mailing lists.


Acknowledgement
---------------

Andreas Westfeld, Technische Universität Dresden
 

Warning
-------

THIS SOFTWARE IS NOT INTENDED FOR ANY COMMERCIAL APPLICATION AND IS PROVIDED
'AS IS', WITH ALL FAULTS AND ANY EXPRESS OR IMPLIED REPRESENTATIONS OR
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED REPRESENTATIONS OR
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, TITLE
OR NON INFRINGEMENT OF INTELLECTUAL PROPERTY ARE DISCLAIMED. IN NO EVENT SHALL
THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE. 
