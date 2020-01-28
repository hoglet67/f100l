#
# Compute digits of e using Rabinowitz & Wagon spigot algorithm 
#
# python3 e.py  [digits]
#
# Can get to 64K without needing more than 16b for the remainder store, but
# need to use 32bit arithmetic to compute the temporary variable n and to be
# able to divide it down to give new (16b) remainder and quotient.
#

import math, sys

if len(sys.argv) >1:
    digits = int(sys.argv[1])
else:
    digits = 1024

max_rem = 0
max_quo = 0
max_n   = 0
## Need slightly more remainder cols than digits to avoid errors in the last
## one or few digits.
cols = digits+2
remainders = ([0] + [1] * cols)
result = ["2."]
for j in range(0,digits-1):
    q = 0
    for i in range (cols,-1,-1) :
        n = q + remainders[i] * 10
        q = n//(i+1)
        remainders[i] = n % (i+1)
        max_rem = max( max_rem, remainders[i])
        max_quo = max( max_quo, q)
        max_n   = max( max_n, n)

    result.append( "%d" % q )
    
result = ''.join(result)

print(result)

print("Cols         : %d" %cols)
print("Max remainder: 0x%04X" % max_rem ) 
print("Max Quotient : 0x%04X" % max_quo ) 
print("Max N        : 0x%04X" % max_n ) 

## Check vs the reference

e_ref =''.join('''
2.718281828459045235360287471352662497757247093699959574966967627724076630353
  547594571382178525166427427466391932003059921817413596629043572900334295260
  595630738132328627943490763233829880753195251019011573834187930702154089149
  934884167509244761460668082264800168477411853742345442437107539077744992069
  551702761838606261331384583000752044933826560297606737113200709328709127443
  747047230696977209310141692836819025515108657463772111252389784425056953696
  770785449969967946864454905987931636889230098793127736178215424999229576351
  482208269895193668033182528869398496465105820939239829488793320362509443117
  301238197068416140397019837679320683282376464804295311802328782509819455815
  301756717361332069811250996181881593041690351598888519345807273866738589422
  879228499892086805825749279610484198444363463244968487560233624827041978623
  209002160990235304369941849146314093431738143640546253152096183690888707016
  768396424378140592714563549061303107208510383750510115747704171898610687396
  965521267154688957035035402123407849819334321068170121005627880235193033224
  745015853904730419957777093503660416997329725088687696640355570716226844716
  256079882651787134195124665201030592123667719432527867539855894489697096409
  754591856956380236370162112047742722836489613422516445078182442352948636372
  141740238893441247963574370263755294448337998016125492278509257782562092622
  648326277933386566481627725164019105900491644998289315056604725802778631864
  155195653244258698294695930801915298721172556347546396447910145904090586298
  496791287406870504895858671747985466775757320568128845920541334053922000113
  786300945560688166740016984205580403363795376452030402432256613527836951177
  883863874439662532249850654995886234281899707733276171783928034946501434558
  897071942586398772754710962953741521115136835062752602326484728703920764310
  059584116612054529703023647254929666938115137322753645098889031360205724817
  658511806303644281231496550704751025446501172721155519486685080036853228183
  152196003735625279449515828418829478761085263981395599006737648292244375287
  184624578036192981971399147564488262603903381441823262515097482798777996437
  308997038886778227138360577297882412561190717663946507063304527954661855096
  666185664709711344474016070462621568071748187784437143698821855967095910259
  686200235371858874856965220005031173439207321139080329363447972735595527734
  907178379342163701205005451326383544000186323991490705479778056697853358048
  966906295119432473099587655236812859041383241160722602998330535370876138939
  639177957454016137223618789365260538155841587186925538606164779834025435128
  439612946035291332594279490433729908573158029095863138268329147711639633709
  240031689458636060645845925126994655724839186564209752685082307544254599376
  917041977780085362730941710163434907696423722294352366125572508814779223151
  974778060569672538017180776360346245927877846585065605078084421152969752189
  087401966090665180351650179250461950136658543663271254963990854914420001457
  476081930221206602433009641270489439039717719518069908699860663658323227870
  937650226014929101151717763594460202324930028040186772391028809786660565118
  326004368850881715723866984224220102495055188169480322100251542649463981287
  367765892768816359831247788652014117411091360116499507662907794364600585194
  199856016264790761532103872755712699251827568798930276176114616254935649590
  379804583818232336861201624373656984670378585330527583333793990752166069238
  053369887956513728559388349989470741618155012539706464817194670834819721448
  889879067650379590366967249499254527903372963616265897603949857674139735944
  102374432970935547798262961459144293645142861715858733974679189757121195618
'''.split())

if result == e_ref[0:len(result)]:
    print("PASS - all digits match")
else:
    for i in range(0,min(len(result),len(e_ref))):
        if result[i] != e_ref[i]:
            print("Error at digit %d, expected %s, actual %s" % ( i, e_ref[i], result[i]))
    if i >len(e_ref):
        print("(Check truncated at %d, which is max digits of reference available here" % len(e_ref))
