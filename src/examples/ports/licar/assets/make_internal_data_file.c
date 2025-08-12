/**
  Helper program to generate the array containing internal data file for
  assets.h. This is done because the C99 standard theoretically limits string
  literal size, and so if we put the huge string right into assets.h, the
  source code might be untranslatable with some compilers. Big compilers like
  GCC have no problem, but we want to maximize compatibility, and so we move
  the "less compatible" code here. This program just converts a string literal
  to a more friendlier array of numbers.
*/

#include <stdio.h>

const char *part1 =                          // big maps
  // MAP 1:
  "MLC1;487 0 "
  ":*C2mL:!x6G:+L2H:+D38"                    // start, finish, CPs
  // pillars:
  ":nw0w2L:f151:m151"
  ":nz0w2L:f151:m151"
  ":nw0H2L:f151:m151"
  ":nz0H2L:f151:m151"
  // big structure:
  ":=z09:f83D"
  ":=H0k:fa1s"
  ":=H0D:fa29"
  ":-A29:f61C"
  ":'A2pI:f611"
  ":'B2j:f511"
  ":=C0j:f347"
  ":=H0q2:f42c"
  ":xB2k:f625"                               // start hole
  ":uB2o:uF2oJ:uB2kL:uF2kI"                  // curved corners in start
  ":]G2kL:]G2oL"                             // decorative ramps near start
  ":-w29:f41e"
  ":xH0r:f35b"                               // hole
  ":=G0C:f561"                               // small wall
  ":^G5CJ:^K5CL"                             // the wall corners
  ":=H0o1:f412"                              // grass near start
  ":=H1L1:fa11"                              // grass
  ":;R0q:f16l"                               // big wall west
  ":;E0ML:fc61"                              // big wall north
  ":)Q1rJ:f11c"                              // curved ramps along big wall
  ":)Q2DJ:f118"                              // curved ramps along big wall
  ":uQ2L|:f141:\\R0LI|:f161:\\Q0ML:f161"     // corner between big walls
  ":=v09:fc2g"
  ":^z3s:^z3tI"                              // two small ramps near finish
  ":;R0l:f125"                               // small wall near start
  ":;L0jJ:f521"                              // small wall near start
  ":uQ1kI:\\R0k:f121:\\Q0jL|:f121"           // corner between small walls
  ":=G0f1:f545:AK0f1:f141"                   // big grass block near start
  ":=A2a2:f12f:^A3a2"                        // wall
  ":=w0o:f331"
  ":=v072:f14i"                              // wall
  ":=G072:f14d"                              // wall
  ":'G1lL:f113:<H1lL:f113"                   // start ramps
  ":/M1z:f311"                               // ramps near start
  ":<G2DL:f118"                              // climb ramps
  ":=v4i:f613:nv4i|:nA4i"                    // roof near the end
  ":'w2n:f411:^x3o:f211"                     // final ramp
  // last turn ramp:
  ":=v062:fc33"
  ":=v052:fc71"
  ":=v362:fc11"
  ":^v462I:^G462I"
  ":'w28I:fa11"
  ":^w37I:fa11"
  ":^w46I:fa11"
  // bridge:
  ":'C2qI:f311"
  ":^C3pI:f311"
  ":<C4oI:f311"
  ":-C4l:f313"
  ":<C4k:f311"
  ":^C3j:f311"
  ":'C2i:f311"
  ":)C3o-:f311"
  ":)C3k-I:f311"
  // finish ramp:
  ":-w5u:f31d"
  ":'w5G:f311"

  // MAP2:
  "#MLC2;809 1"
  // start platform:
  ":-v8k:f213:'v8n:f211:Iv0m2|:f181:.v2l2I:.v4l2I:|x8kL-:f114:xv0k:m3a4"
  // house:
  ":=A3B2:f113:^A4B2L:f113:^B3B2L:f113:;B1B:f122:;A1DL:f121:IB1D:f121:=A1B:f122:m243"
  // overall structure:
  ":\\m3o|I:\\m3n:\\l3nI:\\l3oJ:\\l3pL:\\m3m|I:\\l3m|:m214:xm3j:f118:xl3k:f114" // zigzag wall
  ":=f0n2:fz1t:=f1I2:fz18"
  ":=D0w3:fa1c:=B0w1:f21e"
  ":xA1I:f912:xf0n:f715"
  ":xf0s:fc1a:xC0B:f419"
  ":xL0n:f32c:xf0N:fg23"
  ":-r1s:f418:=i0h2:f63a"
  ":^n2m2L:f114:^l2q2I:f211:vn2q2J:An0q2L:f121:=l0m:m335"
  ":=n0q2:'n0C2L:f116:'o0C2J:f116"
  ":'l1I2L:f115:-m1I2:f215:'o1I2J:f115"
  // corners:
  ":Au0N2I:f121:Av0O2I:f121:Ax0P2I:f121:xv0P:f221"
  ":=q1H2:f911:Ap1H2J:Au1H2J:\\t1H2:Az1H2:AA1J2"
  ":\\q1G2J:\\r1G2|L:\\v1G2J:\\w1G2|L:\\D1J2J:\\E1J2|L"
  ":AF1K2:AG1K2|:AI1J2|:Aq0B2J:Aq0s2I:Al0r2I"
  ":AC0J1:AF0J2|:AC0B1L:AF0B3I:Af0C2J:Af0M2I:f121"
  ":AN0P2L:f121:AN0z2"
  // checkerboard:
  ":=H0A:=J0A:=L0A:=G0B:=I0B:=K0B"
  ":=M0B:=H0C:=J0C:=L0C"
  // grass hill:
  ":vJ2u1:vK2u1|:^J2t1J:^K2t1L"
  ":vJ2s1L:vK2s1I:~H2u1"
  ":vI2t1J:vL1v1J:^L1s1L:f113"
  ":^G1v1I:f511:^G2t1I:f211"
  ":^I2s1L:=G1s1:f513"
  ":=G0s1:f231:m634:~H3r1:xA1n:f435"
  // narrow bridge:
  ":-F1j:f216:=F0j2:f212:-r1j:fg12:=r0j2:f2122:'r1jL:f112"
  ":(E1II:f311:m311:=B0H2:f112:=G0H2:f112" // pipe
  // ring:
  ":=v0r:f161:<w5rL-:]w6rJ:|w7rJ:|x6rL-:]x7rL-"
  ":=x8r:-vbr:<wbrL:]warJ-:|w9rJ-:|xarL:]x9rL"
  ":<ubrJ:]uarL-:|u9rL-:|tarJ:]t9rJ"
  ":=t8r:<u5rJ-:]u6rL:|u7rL:|t6rJ-:]t7rJ-"
  ":<g1G2:f511:'g1H2:f511"    // ramp left
  ":<J1G2:f511:'J1H2:f511"    // ramp right
  // details:
  ":vw1F1:f131:vx1F1|:f131:vw1E1L:f131:vx1E1|J:f131" // tree
  ":~m1D2:~q1E2:~z2I2:~v2M2:~E2K2:~q1o2:~u1p2:~G1z:~H1E" // bumps
  ":;r1rJ:fd11:=u1r:f311:)u2rJ:)w2rL" // wall
  ":;z1s:f117" // house wall
  ":=L0n:f12c"
  // finish house:
  ":;B1rJ:f321:(B3rI:f311"
  ":'B3q:f311:-B3p:f311"
  ":'B3oI:f311:(B3n:f311"
  ":;B1nL:f321:oC0p"
  // CPs and finish:
  ":*v9h:+v8r:+j2K:+F3r:+l3m:!C1p"

  // MAP 3:
  "#MLC3;560 2"
  ":*qrt:!o1y:!o1z:+Wsz"
  // start box:
  ":=mqr:f96d:xnrs:f74b:^mvrJ:f11d:^uvrL:f11d"
  ":~ouz3-:~suz3-:~ouw3-:~suw3-"    // lights
  ":^utuI-:^utv-:^utzI-:^utA-"
  ":'ustI:-usu:-usv:'usw"
  ":'usyI:-usz:-usA:'usB"
  ":xpvA:f313"
  ":upvAL:urvAI:urvCJ:upvC"
  ":opqz:f314"
  ":^pwAI-:f311:^pwz:f311"
  ":^pxBI-:f311:^pxA:f311"
  ":^pyCI-:f311:^pyB:f311"
  ":^pzDI:f311:^pzC:f311"
  // start bridge:
  ":'ovEI:f511:<ouE-:f511"
  ":<ovFI:f511:'ouF-:f511"
  ":^ouGI:f511:^otG-:f511"
  ":^otHI:f511:^osH-:f511"
  ":'osII:f511:<orI-:f511"
  ":<osJI:f511:'orJ-:f511"
  // big flat plane:
  ":=orK:fB16"
  ":>IrL:f514"
  ":^orPI-:fB11"
  ":AKtL2|I:m111:AStP2|I:m111:IPsL2:IVsN2"    // pillars 
  ":-UrC3-:f518:'UrJ3-:f511:'UrC3I-:f511"     // fozen bridge
  ":=Urw2:f516"                               // dirt
  ":|Ysw2J:f11k"                              // fence
  ":=vrK1:f713"
  ":AW0y:f1r1:AW0M:f1r1:Ap0M:f1r1"            // bottom pillars
  // loop:
  ":]FsNJ:f113:|FtNJ:f113"
  ":]FwNJ-:f113:|FvNJ-:f113"
  ":]DsKL:f113:|DtKL:f113"
  ":]DwKL-:f113:|DvKL-:f113"
  ":-DxK:f316"
  ":;CsK|:f153"
  ":;GsN:f153"
  ":,CxK|:f116"
  ":,GxK:f116"
  ":}FsNJ:}DsML:|EsM2:|EsN2I"
  ":ICsPI:f151:IGsK:f151"
  // downhill:
  ":^TrwJ:f116:^TqwL-:f116"
  ":^SqwJ:f116:^SpwL-:f116"
  ":^RpwJ:f116:^RowL-:f116"
  ":^QowJ:f116:^QnwL-:f116"
  ":^PnwJ:f116:^PmwL-:f116"
  ":^OmwJ:f116:^OlwL-:f116"
  ":^NlwJ:f116:^NkwL-:f116"
  ":^MkwJ:f116:^MjwL-:f116"
  ":^LjwJ:f116:^LiwL-:f116"
  ":^KiwJ:f116:^KhwL-:f116"
  ":^JhwJ:f116:^JgwL-:f116"
  ":^IgwJ:f116:^IfwL-:f116"
  ":=Hfw2:f116"
  ":^GfwJ:f116:^GewL-:f116"
  ":^FewJ:f116:^FdwL-:f116"
  ":^EdwJ:f116:^EcwL-:f116"
  ":^DcwJ:f116:^DbwL-:f116"
  ":^CbwJ:f116:^CawL-:f116"
  ":^BawJ:f116:^B9wL-:f116"
  ":^A9wJ:f116:^A8wL-:f116"
  ":^z8wJ:f116:^z7wL-:f116"
  ":^y7wJ:f116:^y6wL-:f116"
  ":yx6wJ:f116:^x5wL-:f116"
  ":yw5wJ:f116:^w4wL-:f116"
  ":yv4wJ:f116:^v3wL-:f116"
  ":^u3wJ:f116:^u2wL-:f116"
  ":^t2wJ:f116:^t1wL-:f116"
  ":^s1wJ:f116:^s0wL-:f116"
  // bumps on downhill:
  ":oDcB:f121"
  ":=Fey:f121"
  ":oJhw:f121"
  ":=KiA:f121"
  ":=Mkx"
  ":=Pny:f121"
  // finish platform:
  ":=n0w:f516:=o0y1:=o0z1"
  ":Ap1x2L:f1p1:Aq1w2L:f141:Ap1w2J:f141"
  ":Ap1A2:f1p1:Aq1B2:f141:Ap1B2I:f141"
  // heart:
  ":\\3031I:\\3021|:=4021:n4031L:A4011J"
  ":\\6031I|:\\6021:=5021:n5031I:A5011"

  // MAP 4:
  "#MLC4;821 1"
  // overall structure:
  ":=000:f53a:x111:f424:x116:f323"
  ":)115L:)315J:)125L-:)325J-:x215:f121"            // start door
  ":-031:fa19:<031:fa11:<039I:fa11:<621I-:f411"     // start roof
  ":A&0hI|:f171:v&7h|:^Z7hI:f211:^@6hJ-:xZ1h:m371"  // big gate
  ":=500:fz16:f51k:=D01:fp16"
  ":_X07:-X02:f41m:=X0u:f51q"
  ":=00y:f51p:=30W:fT14"
  ":=U0T:f414:=W0S:f414:=W0X:=T0V"
  ":^a05-:ft11"
  ":<X1n:f411:<Y1uI:f311:=X0n:f411:'X0m:f411"       // jump ramps
  // bottom section:
  ":=a05:fu21:^a25I:fu11:'v03J:f112:'s15L:'t15J"
  ":,q242-:;q233:-n03:f812:-q041:f311:-n031:f211:mh53"
  ":;41ZJ:fR21"                                     // long wall top
  ":;91WL:fJ21"                                     // long wall top bottom
  ":xr1V:fa37"
  ":n31W:f121:=31X:f123:=01W:f321"                  // corner top left
  // diagonal corner top left:
  ":=30U:f413:=70V:=50T"
  ":A50S:f131:A60T:f131"
  ":A70U:f131:A80V:f131"
  ":A41SI:f121:A51TI:f121"
  ":A61UI:f121:A71VI:f121"
  ":\\81WJ|:f121"
  // top section:
  ":\\w1ZL:f131:\\x1YI|:f131L"
  ":nw4Y1L:f121"
  ":;w1&J:f421:f151"
  ":=w3Z2:^w3@2I-:^w4@2:=w4&2"
  ":=w0Y:f514"
  ":AA0&L:f131:AB0@L:f131:AA1@J:f121:\\B1ZJ:f121"
  ":=w0Y:f141:m664"
  // small house top:
  ":-J3Y:f411:<J3ZI:f411:,K1Z2J:f211:^J0ZI:f311"
  ":=M1Z2:f121:=J0Y1:m443:=I0X2:=J0Y3:x514:f426"
  ":=D06:fk21:nD06I:f121:=D01:fp21:nD11I"           // walls bottom
  ":=W06:f12b:=&01:f12g"
  // top right corners:
  ":AV0ZL:f131:AW0YL:f131:AX0XL:f131"
  ":AY0WL:f131:AZ0VL:f131:A@0UL:f131"
  ":A&0TL:f131:AV1YJ:f121:AW1XJ:f121"
  ":AX1WJ:f121:AY1VJ:f121:AZ1UJ:f121"
  ":A@1TJ:f121:AS0VJ:f131:AT0UJ:f131"
  ":AU0TJ:f131:AV0SJ:f131:AW0RJ:f131"
  ":AT1VL:f121:AU1UL:f121:AV1TL:f121"
  ":AW1SL:f121:AX1RL:f121"
  ":=U1Z:f121:\\S1WL:f121"
  ":=X0C2:f13f:f21f:=&0C2:f13h:=@0C2:f11f"          // right walls
  // left section:
  ":=01P:f127:=41P:f123:\\01O|L:f121:\\41OJ:f121"
  ":\\22L|I:m111:\\42G|I:m111:\\22C|I:m111"
  ":~11G2:=50y:f212:A00yJ:A50AL:A60zL:A70yL"
  // bowl:
  ":'60rJ:f117:^70rL-:f117:^71rJ:f117:|72rJ-:f117"
  ":|82rL:f117:|81rL-:f117:'50rL:f117:m437"
  // details:
  ":;y031J:f311:;y021L:f311"                        // hole
  ":^3212L:^3112L-:-2212:-2112-"                    // arrow
  ":=410:f222:]X1uL:f118:]&1uJ:f118"
  ":>F01:f316:>00D:f513"
  ":=b0W3:fa14:'W02L:f114"
  ":=60I2:f236:^62IJ:f116:^73IJ:f116"               // spectator seats
  ":|01z2L:f11eborderleft"
  ":\\Q121L:f121:\\P121|J:f121:\\Q151|L:f121:\\P151IJ:f121"
  ":\\Q261L:\\P261|J:\\Q211|L:\\P211IJ"
  ":=915:f12f"                                      // wall
  ":n915J:f121:n415:f121"
  ":-6323:f212"
  ":!612:f212:+W1V:+21Q:+D13:*217I"

  // MAP 5:
  "#MLC5;954 0"
  ":*I8wJ"
  ":vq1t1J:m111:vs1r1J:m111"
  ":vI4g3J:AI3g1L:AI2g1L:m131"
  ":vN4g3J:AN3g1L:AN2g1L:m131"
  // overall big structure:
  ":=o0k:fp1x:xu0B:f81b:=C1k:fb25:=I2g:f514"
  ":=I1p:f52j:=50p:fg35:=509:fr1g:xa0h:fe18"
  ":=j0s:f53g:=51n2:f512:xH0I:fa1a"
  ":]o1uL:f119:|o2uL:f119" // ramp left
  ":)H1qJ:f11d"            // ramp right
  ":xb0f:fb12:xd0e:f711:x509:f313:f611:f116"
  ":xm09:fc11:xs0a:f516:xu0g:f212:xp0a:f312:xr0c"
  ":An0hI:Am0gI:Al0fI:Aj0eI:Aa0hL:Ab0fL:Ad0eL"
  ":Aw0j:Av0i:Au0h:At0g:As0f"
  ":Ar0d:Aq0c:Ap0b:Ao0a:Am09"
  ":A50eJ:A60cJ:A70bJ:A80aJ:Aa09J"
  ":=i0u:f131:Ah0uI:f131:Ai0vI:f131"
  ":xo0I:f11a:xp0N:f114:xp0Q:f411:xD0Q:f411:xG0N:f114"
  ":Au0L:AB0LJ:Ap0NI:Aq0PI:As0QI:AD0QL:AF0PL:AG0NL"
  ":nM0HL:f131:>x0r:f412"
  ":=u0l1:f414:ou0A:f811"
  // corner:
  ":xo0l:f218:xq0n:f214"
  ":Aj0p:f131:Ak0q:f131:Al0r:f131:Am0s:f131"
  ":An0t:f131:xk0p:f131:xn0s:f131"
  ":Ao0tJ:Ap0sJ:Aq0rJ:Ar0qJ:As0pJ"
  ":As0oI:Ar0nI:Aq0mI:Ap0lI:Ao0kI"
  // diagonal bridge:
  ":=82x:f919:=52u:f713:=g2C:f316"
  ":A52wI:A62xI:A72yI:A82zI:A92AI:=72x"
  ":Aa2u:Ab2v:Ac2w:xb2u"
  ":Ag2HI:Af2GI:Ae2FI:Ad2EI"
  ":Ae2y:Af2z:Ah2B:Ai2C"
  ":xg2z:xe2x:xf2x:f212"
  ":A92BJ:A72DJ:=80C1:f141:x82B:x82A"
  ":Ac2EL:Aa2GL:=b0F1:f141:xc2F:xd2F"
  ":A72EI:A82FI:A92GI"
  ":=d0x1:f141:=g0A1:f141"
  // tunnel:
  ":=I3q:f56d:xJ3q:f35d:^I8qJ:f11d:^M8qL:f11d"
  ":]J3qL:f11d:]L3qJ:f11d"
  ":|J4qL:f11d:|L4qJ:f11d"
  ":|J6qL-:f11d:|L6qJ-:f11d"
  ":]J7qL-:f11d:]L7qJ-:f11d"
  ":=H0w:f181:AH1xI:f171:AH1v|:f171:xI8v:f213:AJ8x|:AJ8vI" // start platform
  // big bridge:
  ":<o3DJ:f115:=o1D:f125"
  ":'p3DJ:f115:<p2DL-:f115"
  ":^q4DJ:f115:^q3DL-:f115"
  ":^r5DJ:f115:^r4DL-:f115"
  ":'s5DL-:f115:<s6DLJ:f115"
  ":<t5DL-:f115:'t6DJ:f115"
  ":=u6D1:f815"
  ":<C5DJ-:f115:'C6DL:f115"
  ":'D5DJ-:f115:<D6DL:f115"
  ":^E5DL:f115:^E4DJ-:f115"
  ":^F4DL:f115:^F3DJ-:f115"
  ":'G3DL:f115:<G2DJ-:f115"
  ":<H3DL:f115:=H1D:f125"
  ":Iu0D1:f161:Iu0H1J:f161:IB0D1|:f161:IB0H1L:f161"  // pillars
  ":<A1kJ:f115:'B1kJ:f115:<C2kJ:f115:'D2kJ:f115"     // big ramp right
  ":<51l2:f511:'51m2:f511:<52n2:f511:'52o2:f511"     // big ramp left
  ":+K7w:+y1O:+k3s:+g1b:+c3B:!93E:!K3h"
  // house:
  ":;j3w2:f127:;n3w2|:f127"
  ":-j5w:f517"
  ":,j4x-:f112:,j4A-:f112"
  ":,n4x|-:f112:,n4A|-:f112"
  ":'j5w2L:f117:'n5w2J:f117"
  // details:
  ":)p1KL:f113:)G1KJ:f113"                           // ramps
  ":|u1A2:f811:|u7DI:f811:|u7H:f811"                 // small walls
  ":(c3pI:|c3o-:(g3pI:|g3o-"                         // shafts
  ":;c3tJ:f511"                                      // small wall
  ":~n1g1:~o1f2:~p1e2:~q1d1"
  ":nw4n2L:Iw3n:m121"                                // column
  ":'w0sJ:m111:'C0sJ:m111:}z0s2J|:m111"              // pits
  ":=c2q3:f513"                                      // ice
  ":;g082L:f151:;g0e2J:f151:-g58:f117:~g493-:~g4d3-" // gate
  ;

const char *part2 =                                  // small maps
  /* tiny maps, max:
    - 400 character string
    - 512 (0x200) blocks
    - 1024 (0x400) vertices
    - 1536 (0x600) triangles */

  // TINY MAP 1:
  "#MLCtiny1;330 0:*G1b:+n9H:!I1H"
  // start
  ":=E0b:f61i"
  ":^D1bJ:f11i:^J1bL:f11i"
  ":^D0bJ-:f11i:^J0bL-:f11i"
  ":^E1s:f511"
  ":>E0k:f514"
  // slope:
  ":^C2vI-:f611"
  ":^p3v:fj11"
  ":^o4w:fj11:^o3w-I:fj11"
  ":^n5x:fj11:^n4x-I:fj11"
  ":^m6y:fj11:^m5y-I:fj11"
  ":^l7z:fj11:^l6z-I:fj11"
  ":^k8A:fj11:^k7A-I:fj11"
  ":=k8B:f419:=o8F:f415:^k9K2I:f611:^k8K2I-:f611" // top
  ":=y0F1:fb15:^y0F1:fb11:^y0J1I:fb11"            // end

  // TINY MAP 2:
  "#MLCtiny2;175 1"
  ":*w@v-:!v0u:f313"
  // tunnel:
  ":=wct:f1K1:=wcx:f1K1:=ucv:f1K1:=ycv:f1K1"
  ":Avcw:f1K1:AucwI:f1K1:AvcxI:f1K1"
  ":AxcwJ:f1K1:AycwL:f1K1:AxcxL:f1K1"
  ":AvcuL:f1K1:AucuJ:f1K1:AvctJ:f1K1"
  ":AxcuI:f1K1:Aycu:f1K1:Axct:f1K1"
  ":xukt:f6c6:xuEt:f6c6" // splits
  //obstacles:
  ":~wUv3-:~wVv3:\\xDv2J|:.vQv:,wAu1"
  ":f113:~xnw3-:~xow3:~weu3-:~wfu3:\\vcv2L|"

  // TINY MAP 3:
  "#MLCtiny3;381 2"
  ":*w1d:!w1d:+w1A"
  // big bumps:
  ":vw1hJ:m111"
  ":vw1qJ:m111"
  ":vw1yJ:m111"
  ":vy1r1J:m111"
  ":vy1u2J:m111"
  ":vv1t3J:m111"
  ":vu1k1I:vu1l1|"
  ":vu1v2I:vu1w2|"
  ":vy1lL:vy1m"
  ":vy1v1L:vy1w1"
  ":vy1yL:vy1z"
  // diagonal column:
  ":Aw1mL:m111"
  // big structure:
  ":=t0d2:f71p"
  ":=t1B1:f711"
  ":^t1d1J:f11p:^z1d1L:f11p"
  ":'u0n1I:f511:'u0o1:f511"
  // small bumps:
  ":~y1i1:~w1j3:~x1l:~x1p3:~u1q2:~w1s2:~x1v:~v1w1:~x1x3"

  // TINY MAP 4:
  "#MLCtiny4;172 2"
  ":*M1AJ"
  // box:
  ":=C2w:faca"
  ":=C6w2:fa1a"
  ":=H2w2:f1ca"
  ":=C0z:fb14"
  ":xA2x:fgo8"
  ":^C2w-:fb11:^C1x-:fb11:^C0z-:fb11"
  ":^C0C-I:fb11:^C1E-I:fb11:^C2F-I:fb11"
  ":vC2wL-:vC2F-:vM2wI-:vM2FJ-"
  // top:
  ":-Few2:f411:-FeF2:f411"
  ":AC3w|:f1b1:AM3w:f1b1"
  ":AC3FI:f1b1:AM3FL:f1b1"
  // ramps:
  ":]C2xI:fb11:|C3xI:fb11"
  ":'C1yI:fb11"
  ":<C1zI:fb11"
  ":]C2E:fb11:|C3E:fb11"
  ":'C1D:fb11"
  ":<C1C:fb11"
  // finish:
  ":!Hgx:!Hgw:!HgE:!HgF"

  // TINY MAP 5:
  "#MLCtiny5;296 1"
  ":*x1q:+u1F:!x1o"
  // platform:
  ":=j0o:fg1j"
  ":xj0o:fc18"
  ":xj0E:f613"
  ":=p0w2:f71b"
  ":Aj0DI"
  ":Ap0G2I"
  ":Aj0wJ"
  // walls:
  ":^m1A1I:fd11"
  ":=m1w:f911"
  ":^v1oJ:f118"
  ":Av1wL"
  ":=t0A:f234"
  ":^z1oL:f11j"
  ":^z0oL-:f11j"
  // fans:
  ":Vm1xI:Vm1z"
  ":-w0q1:-y0q1"

  "#" // separate the subsequent user data file
  ;

int main(void)
{
#define ROW_LENGTH 16

  unsigned long count = 0;

  puts("// data generated by make_internal_data_file.c:");
  puts("#if !LCR_SETTING_ONLY_SMALL_MAPS");

  while (*part1)
  {
    printf("0x%2x,",*part1);
    part1++;

    count++;
    
    if (count >= ROW_LENGTH)
    {
      putchar('\n');
      count = 0;
    }
  }

  puts("\n#endif // !LCR_SETTING_ONLY_SMALL_MAPS");

  count = 0;

  while (*part2)
  {
    printf("0x%2x%s",*part2,part2[1] ? "," : "");
    part2++;

    count++;
    
    if (count >= ROW_LENGTH)
    {
      putchar('\n');
      count = 0;
    }
  }

  return 0;
}
