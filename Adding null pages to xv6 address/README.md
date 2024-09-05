Project #3
A
 
-
 
Adding NULL pages to xv6 address 
spaces
 
!
!
"
!
 
1. 
Intro To xv6 Virtual Memory
 
In this project, youÕll be changing xv6 to support a feature virtually
 
every m odern O S 
does: ca
using an exception to occur w hen your program
 
dereferences a null 
pointer 
and
 
adding the ability to change the protection
 
le v e ls o f so m e  p a g e s in  a  p ro c e ssÕs 
addressspace.
 
 
 
2. 
Null
-
pointer
 
Dereference
 
In xv6, the V M  
system  u ses a  sim ple tw o
-
le v e l p a g e  ta b le  a s d isc u sse d  in
 
class. A s it 
currentlyisstructured,usercodeisloadedintotheveryfirst
 
partoftheaddressspace. 
Thus, if you dereference a null pointer, you will
 
not see an exception (as you m ight 
expect);
 
ra ther, yo u w ill see w ha tever c o d e
 
is th e  fir st b it o f c o d e  in  th e  p r o g r a m  th a t 
is r u n n in g . T r y  it a n d  se e !
 
 
Thus, the first thing you m ight 
wan
t to  
do is 
to  
create a program  that dereferences a
 
null pointer. It is sim ple! See if you can do it. Then run it on Linux 
as well
 
as xv6, to 
see th e d ifferen c e.
 
 
Yourjobherewillbetofigureouthowxv6setsupapagetable.Thus,once
 
again,this 
project is m ostly about understanding the code, and not w riting
 
very m uch. Look at 
how
 
exec()
 
works to better understand how ad
dress
 
spa c es g et filled  w ith  c o d e a n d  in  
generalinitialized.
 
 
You should also look 
at
 
fork
()
,
 
in  p a r tic u la r  th e  p a r t w h e r e  th e
 
address space of the 
child is created by copying the address space of the
 
parent. W hat needs to change in 
th e re ?
 
 
Therestof 
yourtaskw illbecom pletedbylookingthroughthecodetofigure
 
outwhere 
th e re  a re  c h e c k s o r a ssu m p tio n s m a d e  a b o u t th e  a d d re ss sp a c e . T h in k
 
about
 
what 
happens w hen you pass a param eter into the kernel, for exam ple; if
 
passing a
 
pointer, 
th e k e rn e ln e e d
sto bev eryc a refu lw ith it,to en su reyo u
 
havenÕtpassedita
 
badpointer. 
Howdoesitdothisnow?Doesthiscodeneed
 
to c h a n g e in o rd e rto
 
workinyournew 
versionofxv6?
 
 
One last hint:
 
youÕll have to look at the xv6 m akefile as w ell. In there
,
 
user 
program s 
are com piled so as to set their entry point (where the first
 
in str u c tio n  is)  to  0 . I f y o u  
change xv6 to m ake the first page invalid,
 
clearly the entry point w ill have to be 
so m ew h ere else (e.g ., th e n ex t pa g e,
 
or 0x1000). Thus, som ething in the m a
kefile w ill 
needtochangetoreflect
 
th is a s w e ll.
 
 
 
 
3. 
Read
-
only Code
 
¥
!
In m ost operating system s, code is m arked read
-
only instead of
 
rea d
-
write. 
However,
 
in  x v 6  th is is n o t th e  c a se , so  a  b u g g y  p r o g r a m  c o u ld
 
accidentally 
overwriteits 
owntext.Tryitandsee!
 
 
¥
!
In this portion of the xv6 project, youÕll change the protection bits of parts
 
of the 
page table to be read
-
only, thus preventing such over
-
writes, and also
 
be able to 
changethem back.
 
 
¥
!
Todothis,youÕllbeadding 
tw o
 
system  c a lls:
 
 
int
 
mprotect
(
void
 
*addr, 
int
 
len)
 
int
 
munprotect
(
void
 
*addr, 
int
 
len)
!
!
 
¥
!
Calling
 
mprotect()
 
sh o u ld  c h a n g e th e pro tec tio n  
bits of the page range
 
sta rtin g
 
at
 
addr
 
and of
 
le n
 
pages to be read only. Thus, the program  could
 
still rea d  th e 
pages
 
in th isr a n g e a fte r
 
mprotect()
 
fin ish e s,b u ta w rite to
 
th isre g io n sh o u ld c a u se  
a 
tr a p
 
(a nd  thus kill the pro cess). T he
 
munprotect()
 
call does the opposite: sets the 
reg io n
 
backtoboth 
rea d a b le
 
and 
writeable
.
 
 
¥
!
Alsorequired:
 
th e p a g e p ro te c tio n ssh o u ld b e in h e rite d o n fo rk
().T hus,if
 
aprocess 
has 
mprotected
 
so m e o f its pa g es, w h en  th e pro c ess c a lls fo rk, th e
 
OS should copy 
th o se  p ro te c tio n s to  th e  c h ild  p ro c e ss.
 
 
¥
!
Therearesom efailurecasestoconsider:
 
if
 
addr
 
is n o t p a g e  a lig n e d ,
 
or
 
addr
 
pointstoaregionthatisnotcurrentlyapartoftheaddress
 
spa c e, o r
 
le n
 
is 
le ss th a n  o r e q u a l to  z e ro , 
return 
-
1
 
anddonotchange
 
anything.O therwise, 
return 
0
 
uponsuccess.
 
 
 
¥
!
Hint:
 
A
fte rc h a n g in g a p a g e
-
ta b le e n try ,y o u n e e d to m a k e su re th e h a rd w a re
 
know s 
of the change. O n 32
-
bit x86, this is readi
ly  a c c o m p lish e d  b y  u p d a tin g
 
th e
 
CR3
 
reg ister (w ha t w e g eneric a lly c a ll the
 
page
-
ta b le  b a s e  r e g is te r
 
in
 
class). 
Whenthehardwareseesthatyou 
had 
overwr
i
t
t
e
n
 
CR3
 
(evenw iththesa m e
 
value), 
it g u a r a n te e s th a t y o u r  P T E  u p d a te s w ill b e  u se d  u p o n  su b se q u e n t
 
access
es. 
The
 
lc r3 ()
 
fu n c tio n  w ill h e lp  y o u  in  th is p u rsu it.
 
 
 
 
4
. 
Handling Illegal Accesses
 
In both the cases above, you should be able to dem onstrate w hat happens w hen
 
th e  
user
 
codetries 
to
:
 
(a )
!
accessanullpointeror 
 
(b)
!
overwritean 
mprotected
 
reg io n o f m em o ry.
 
Inbothcases,xv6shouldtrapandkilltheprocess(this
 
willhappenwithouttoom uch 
tro u b le  o n  y o u r p a rt, if y o u  d o  th e  p ro je c t in  a
 
sen sible w a y).
 
 
 
5
. Running Te
sts
 
¥
!
Use 
th e  fo llo w in g  s
criptfileforr
unning
 
th e
 
te sts:
 
prompt> ./test
-
null
-
pages
.sh
!
!
¥
!
Ifyouim plem entedthingscorrectly,youshouldgetsom enotification
 
th a tth e te sts 
passed.IfnotÉ
 
 
¥
!
The tests assum e that xv6 source 
code is found in the
 
src/
 
su bd irec to ry.
 
If itÕs not
 
th e re , th e  sc rip t w ill c o m p la in .
 
 
¥
!
The test script does a one
-
tim e  c le a n  b u ild  o f y o u r x v 6  so u rc e  c o d e
 
using a new ly 
generatedm akefilecalled
 
Makefile.test
.Y o u c a n u se
 
th isw h e n d e b u g g in g ( a ssu m in g  
youev
erm akem istakes,thatis),e.g.:
 
 
prompt> cd src/
 
prompt> make 
-
f Makefile.test qemu
-
nox
!
!
¥
!
You can suppress the repeated building of xv6 in the tests with the
 
Ô
-
s
Õ
 
fla g . T h is 
sh o u ld  m a ke repea ted  testin g  fa ster:
 
prompt> ./test
-
null
-
pages
.sh 
-
s
!
!
¥
!
Youcanspecificallyrunasingletestusingthefollowingcom m and
 
 
./test
-
mmap.sh 
-
c 
-
t 
7
 
-
v
!
Thespecificallyrunsthetest_7.calone.
 
!
¥
!
Theother 
usualtestingflags
 
arealsoavailable.See
 
th e  
te stin g  
appendix
 
fo r 
more 
details.
 
 
¥
!
Th
i
s 
projectw
ill h a v e
 
fe w
 
hiddentestcases
 
ap
artfrom the 
providedtestcases
.
 
 
 
6
. 
Addi
ng test files inside x
v6
 
¥
!
Inorder 
to  
run the test 
file s in sid e  x v 6 , m a n u a lly  c o p y  th e  te st file s
(
te s t_ 1 .c , te s t
_2.c
 
etc
É
)
 
in sid e  
xv6
/
s
rc d irec to ry.
(
preferably
 
in a d iffe r e n
tn a m e li
ke 
x
v6test_1.c
,e tc
É
)
 
¥
!
Makethenecessarychange
s to  th e M a kefile
 
UPROGS=
\
 
    
_cat
\
 
    
_echo
\
 
    
_forktest
\
 
    
_
xv6
test_1
\
 
    
_grep
\
 
    
_init
\
 
    
_kill
\
 
    
_ln
\
 
    
_ls
\
 
    
_mkdir
\
 
    
_rm
\
 
    
_sh
\
 
    
_stressfs
\
 
    
_usertests
\
 
    
_wc
\
 
    
_zombie
\
!
!
 
EXTRA=
\
 
    
mkfs.c 
xv6
test_1
.c
 
ulib.c user.h cat.c echo.c forktest.c 
grep.c kill.c
\
 
    
ln.c ls.c mkdir.c rm.c stressfs.c usertests.c wc.c zombie.c
\
 
    
printf
.c umalloc.c
\
 
    
README dot
-
bochsrc *.pl toc.* runoff runoff1 runoff.
list
\
 
    
.gdbinit.tmpl gdbutil
\
!
 
¥
!
Now 
com p
ile  th e  x v 6  
us
in g  t
he 
com m and 
Ò
makeclean&&ma
keq
em u
-
nox
Ó
. 
 
 
prompt> make 
cl
ean && make
 
qemu
-
nox
!
¥
!
O
nce it has c
om p
ile
d succe
ssfu lly
 
and you are inside xv6
 
prom p
t
, y o u  c a n  r u n  th e  
te st.
 
$ 
 
$ 
xv6test
 
 
!
¥
!
Youcanalsoadd
 
yourow ntestca
ses to  
te st y o u r so lu tio n  e x te n siv e l
y
.
 
¥
!
Once you are inside xv6 qemu prompt in yo
ur term inal, if you w ish to shutdow n 
xv6a
ndexit 
qem uusethefollow ingkeycom binatio
ns:
 
¥
!
pressC trl
-
A,thenreleaseyourkeys,thenpressX.(NotCtrl
-
X
)
 
 
 
 
 
 
 
 
 
 
 
 
Appendix 
Ð
 
Test options
 
 
The
 
ru n
-
te sts.sh
 
sc rip tisc a lle d b y v a rio u ste ste rsto d o th e w o rk o f 
te stin g .E a c h  
te st is a c tu a lly  fa ir ly  sim p le : it is a  c o m p a r iso n  o f sta n d a r d  o u tp u t a n d  sta n d a r d  
error,aspertheprogram specification.
 
 
In a ny g iven pro g ra m  specifica tio n d irecto ry, there exists a  
sp e c ific
 
te sts/
 
directory w hich holds the expected retur
n code, standard output, 
and standard error in files called
 
n.rc,
 
n.out, and
 
n.err
 
(respec tiv ely) fo r ea c h  
te st
 
n. T he testing fram ew ork just starts at
 
1
 
and keeps increm enting tests until 
it c a n 't fin d  a n y  m o r e  o r  e n c o u n te r s  a  fa ilu r e . T h u s , a d d in g  n e w  te
sts is e a sy ; 
ju s t a d d  th e  r e le v a n t file s  to  th e  te s ts  d ir e c to r y  a t th e  lo w e s t a v a ila b le  n u m b e r .
 
 
Thefilesneededtodescribeatestnum ber
 
n
 
are:
 
¥
!
n.rc:T hereturncodetheprogram shouldreturn(usually0or1)
 
¥
!
n.out:T hestandardoutputexpectedfrom  
th e  te st
 
¥
!
n.err:T hestandarderrorexpectedfrom thetest
 
¥
!
n.run:H ow torunthetest(w hichargum entsitneeds,etc.)
 
¥
!
n.desc:A shorttextdescriptionofthetest
 
¥
!
n.pre
 
(o ptio n a l):C o d eto ru n befo reth etest,to setso m eth in g u p
 
¥
!
n.post
 
(o ptio n a l):C o d eto
 
ru n a fte rth e te st,to c le a n so m e th in g u p
 
 
There is also a single file called
 
pre
 
which gets run once at the beginning of 
te stin g ;th isiso fte n u se d to d o a m o r e c o m p le x b u ild o fa c o d e b a se ,fo r e x a m p le . 
To prevent repeated tim e
-
wasting pre
-
te st a c tiv it
y, suppress this w ith the
 
-
s
 
fla g  
(a sd esc ribed belo w ).
 
 
In m o st ca ses, a  w ra pper script is used  to  ca ll
 
run
-
te s ts .s h
 
to  d o  th e  n e c e ssa r y  
work.
 
 
Theoptionsfor
 
ru n
-
te sts.sh
 
in c lu d e :
 
¥
!
-
h
 
(th eh elpm essa g e)
 
¥
!
-
v
 
(v erbo se:prin tw h a tea c h testisd o in g )
 
¥
!
-
t n
 
(r
unonlytest
 
n)
 
¥
!
-
c
 
(c o n tin u eev en a ftera testfa ils)
 
¥
!
-
d
 
(ru n testsn o tfro m
 
te sts/
 
directorybutfrom thisdirectoryinstead)
 
¥
!
-
s
 
(su ppressru n n in g th eo n e
-
tim e  se t o f c o m m a n d s in
 
pre
 
file )
 
 


P r oj ect #3
B
 
-
 
xv 6 Ker n el  Thr eads
 
 
 
1 .  
Ke r n e l Th r e ads
 
 
 
We ll,  it 
sh ou ld.  B e ca u se  y ou  a re  on  y ou r w a y  t o b e comi n g  a  re a l k e rn e l
 
h a ck e r.  An d w h a t 
cou ldb e more f u n t h a n th a t ?
 
 
 
 
 
t o cre a t e  a 
k e rn e l t h re a d,  ca lle d
 
clon e ( )
,  a s w e ll a s on e  to w a i t  f or a
 
t h rea d 
ca lle d
 
j oi n ( ).
 
Th e
n, 
 
 
clon e ( )
 
t o b ui ld a  li t t le t h re a d
 
li b ra ry,  wi t h  a
 
t h re a d_cre at e ( )
 
ca ll 
a n d
 
lock _a cqui re( )
 
 
2 .  
Ove r vi e w
 
You rn e w clon e sy st e m ca llsh ou ldlook li k e th i s:
 
 
 
int
 
c l on e
(
v oi d
( * fc n ) (
v oi d
*, 
void
 
* ),  
void
 
* a r g 1,  
v o id
 
* a r g2 ,  
void
 
*
st a c k
)
 
 
 
 
 
a ddre sssp a ce . 
F i le de scri p t ors a re  cop i e d a s i n
 
f ork ( ) .
 
Th e  ne w  p roce ss u se s
 
st a ck
 
a s i t s u se r st a ck , 
w h i ch  i s pa ssed 
w it h  
t w o a rgu me nt s (
a rg1
 
a n d
 
a rg 2
)  a n d
 
u se s 
a  fa k e  re t u rn  PC 
(
0 xf ff f f ff f
);  a p rop e r t h rea d w i ll si mp ly  ca ll
 
e xit ( )
 
pa g e
-
a lig n e d.  Th e  ne w  t h re a d sta rt s 
e xecu t i ng  at  t h e a ddre ss sp e ci f ie d b y
 
f cn
.  Asw i th
 
f ork ( ) ,
 
th e  PID 
of  t h e n e w t h re a d i s 
re t u rn e dt ot h e p a re nt
 
( f orsi mp li cit y, t h re a dsea ch h a ve th e i row n p roce ssID) .
 
 
Th e ot h e rn e w sy st e m ca lli s
:
 
 
int
 
j o in
(
v o id
 
**
st a c k
)
 
 
 
Th i sca llw ai t sf ora
 
ch i ldt h rea dt ha t sha re st he a ddre sssp a ce w it h t he ca lli ng
 
p roce ss 
to
 
e xi t .  It  re t u rn s th e  PID of  w a it e d
-
f or ch i l
d or 
-
1  i f  n on e .  Th e  loca t i on  of
 

u se rst a ck i scop i e di nt ot h e a rgu me nt
 
st a ck
 
(wh i ch ca n t he n b e
 
f re e d).
 
 
You  a lso n e e d t o t h i
n k  a b out  t h e  se ma nt i cs of  a  cou p le of  e xi st i n g  sy ste m
 
ca lls.  F or 
e xa mp le,
 
i n t  wa i t( )
 
sh ou ld w a it  f or a  ch i ld p roce ss t h a t  doe s  n ot
 
sh a re  t h e  a ddre ss  
 
2
 
sp a ce wi t h t h i sp roce ss. It sh ou lda lsof re e th e a ddre ss
 
sp a ce if t hi si s 
t h e  
la st
 
re f e re n ce 
t oi t . Also,
 
e xi t ()
 
sh ou ldw ork a s 
b e f ore
 
b ut f orb ot h p roce ssesa n dt h re a ds; lit t lech an g e 
i sre qu i re dh e re .
 
 
You rt h re a dli b ra ry wi llb e b ui lt on t op of t hi s,a n dj u st ha ve a si mp le
 
rou t i ne .
 
 
 
int
 
t h re a d _ cr e a t e
(
void
 
( * s t ar t _ r ou ti n e ) (
v oi d
 
* , 
void
 
*), 
vo i d
 
*arg1, 
void
 
* a r g 2)
 
 
 
Th i s 
rou t i ne  sh ou ld ca ll
 
m a lloc( )
 
t o cre a t e  a  ne w  u se r sta ck ,  u se
 
clon e ( )
 
 
t o cre at e  t he 
ch i ldt h re a da n dg e t i t ru n ni ng . It re t u rn st h e ne w ly
 
cre a t e dPIDt ot h e p a re n t a n d0 t o 
t h e  ch i ld (i f  su ccessf u l),  
-
1  ot h e rwi se .
 
 
An
 
 
int
 
t h re a d _ jo i n
()
 
 
ca llsh ou lda lso b e cre a t e d, w h i ch ca llst h e  
u n de rly i n g
 
j oin ( )
 
sy st e mca ll, f re e st h e u se r 
st a ck , a n dt h en re t u rn s. It re t u rn sth e
 
w ai t e d
-
forPID( w h e n su cce ssf u l),  
-
1 ot h e rwi se .
 
 
You rt h re a dli b ra ry sh ou lda lsoh a ve a si mp le
 
tick et lo ck
 
( re a d
 
t
his book chapter
 
f o rmo re  
i n f orma ti on on t h i s) . Th e re sh ou ldb e a t y pe
 
lock _t
 
t h at on e u se st o
 
de cla re a l ock , a n d 
t w orou t i n e s
:
 
 
void
 
l oc k _ a cq u i r e
(
l o c k _t
 
*)
 
 
void
 
l oc k _ r el e a s e
(
l o c k _t
 
*)
 
 
w h i ch a cqu i re  an d re lea se  th e  lock .  Th e  sp in  lock
 
sh ou ld u se  x86  a t omi c a dd t o b ui ld 
t h e  lock  

 
se e
 
t hi s  wi ki p edi a  p ag e
 
f or a wa y  t o cre a t e  a n  a t omi c f e t ch
-
a n d
-
a dd rou t i ne 
u si ng t h e x86
 
xa ddl
 
i n st ru ct i on .  
 
 
On e  
la st rou t i n e,
 
 
 
void
 
l oc k _ i ni t
(
l oc k _ t
 
*)
 
 
i su se dt oi n i ti a li ze
 
t he lock a sn e e db e (i t sh ou ldon ly b e ca lle db y on e t h rea d) .
 
 
Th e  t h rea d li b ra ry  sh ou ld b e  a va i la b le a s p a rt  of e ve r
y  p rog ra m th a t ru n s i n
 
xv6 .  Th u s,  y ou  sh ou ld a dd p rot ot y p e s t o
 
u se r/ u se r. h
 
an d t h e  a ct ua l code  t o
 
i mp lem en t t h e lib ra ry rou t in e si n
 
u se r/ u lib . c
.
 
 
 
3
 
On e t h i n
g y ou n e e dt ob e ca re f u lw i t h i s
,
 
w hen a n a ddre sssp a ce i sg row n b y a
 
t h re a d  
i n  a  mu lt i
-
th re a de d p roce ss ( f or e xa mp le,  w he n
 
ma lloc( )
 
i s ca lle d,  i t
 
m
a y  ca ll
 
sb rk
 
t o 
g row t h e a ddre sssp a ce of th e p roce ss) . Tra ce th i scode
 
p a t h ca re fu lly an dse e w h e re a 
n e w  lock  i s n e e de d a n d w h a t  e lse n e e ds t o b e
 
u p da te d t o g row  a n  a ddre ss sp a ce  i n  a  
mu lt i
-
t h rea de dp roce sscor re ct ly .
 
 
 
 
3 .  
Bu i l din g
 
cl on e () 
f r o m
 
fo r k()
 
To 
i mp lem e nt
 
clon e ( ) ,
 
y ou  sh ou ld stu dy  (a nd most ly  cop y )  t h e
 
f ork ( )
 
sy st e m
 
ca ll.
 
Th e
 
f ork ( )
 
sy st em ca ll w i ll se rve  a s a  t emp late  f or
 
clon e( ) ,
 
w i t h
 
some  modif i ca ti on s. 
F ore xa mp le, in
 
k e rn e l/ p roc. c
, w e se e  
t he b eg in n i ng of
 
the
 
f ork ()
 
i mp leme n ta t i on :
 
 
 
 
 
 
 
int
 
fork
(
v oi d
)
 
{
 
  
int
 
i,  p i d;
 
  
s t r u ct
 
p r oc
 
*
np
;
 
 
  
/ /  A ll o c a te  p r oc e s s .
 
  
if
( ( np  =  al l o c pr o c ( ) ) = =  
0
)
 
    
r e tu r n
 
-
1
;
 
 
  
/ /  C op y  p ro c e s s s t a t e f r o m p .
 
  
if
( ( np
-
> p gd i r  = c o p y uv m ( p ro c
-
> pg di r ,  p r oc
-
> s z) )  = = 
0
){
 
    
k f re e
(np
-
> k s ta c k ) ;
 
    
np
-
> k s t ac k  =  
0
;
 
    
np
-
> s t a te  =  UN U S E D ;
 
    
r e tu r n
 
-
1
;
 
  
}
 
  
np
-
> sz  =  pr o c
-
>s z ;
 
  
np
-
> pa r e n t =  p ro c ;
 
  
*np
-
>t f  = * p r o c
-
>tf;
 
 
 
4
 
Th i s code  doe s some  w ork y ou  n e e d t o h a ve  done  f or
 
clon e ( ) ,
 
f or e xa mp le,
 
ca
lli n g
 
a llocp roc( )
 
t oa lloca t e a slot i n t h e p roce sst a b le, cre a ti ng a
 
k e rn e lst a ck f ort h e 
n e w t h rea d, e t c.
 
 
H ow e ve r, a sy ouca n see , th e ne xt th i ng
 
f ork ( )
 
doe si scop y th e a ddre ss
 
sp a ce a n dp oin t 
t h e  pa g e  di re ct ory  (
np
-
>p g di r
)  t o a  n ew  pa ge  t ab le f or t h at
 
a ddre ss spa ce .  Wh en 
cre a t i n g  a  t h re a d ( a s
 
clon e ( )
 
 
 
n ew  ch i ld t h re a d t o b e  i n 
the
 
s am e
 
a ddre ss sp a ce  a s t h e  p a re nt ;  t h u s,  t he re
 
i s n o n e e d t o  cre a t e  a  cop y  of  t h e  
a
 
 
np
-
> pg di r
 
sh ou ld b e  t h e  s
 

 
t h e y n ow sh a re t he a ddre ss
 
sp a ce , an dt h u shave t h e sa me pa ge t ab le.
 
 
 

 
i n si de
 
clon e ( )
 
 
 
k e rn e l st a ck  so 
t h a t wh e n
 
cl
on e ( )
 
re t u rn sin
 
t h e ch i ld( i . e. , i n th e n e w ly
 
cre a t e dt h rea d) , i t ru n son t h e 
u se r st a ck  pa ssed i nt o clon e  (
st a ck
) ,  th a t
 
t h e  f u n ct i on
 
f cn
 
i s t h e  st a rt in g  p oi n t  of  t h e 
ch i ldt h re a d, a n dt h a t t he
 
a rg u me nt s
 
a rg1
 
a n d
 
a rg 2
 
a re a vai lab let ot h a t f u n cti on . Th i s 
w i llb e a
 
lit t lew ork on y ou rp a rt t of ig u re ou t ; h a ve f
un !
 
 
 
4 .  
x8 6 C al l in g Co n ven t io n
 
 
 
ca llin g 
con ve n t i on,  an d e xa ct ly  h ow t h e st a ck  w ork s w h e n  ca lli ng  a f un ct i on .
 
You  ca n re a d 
a b ou t  th i s i n
 
P r ogr amming  Fr om The Gr oun dU p
,
 
a  f re e  on li n e  b ook .  S p e ci fi ca lly,  y ou 
sh ou ldu n de rst a n dCh a p t e r4 ( a n dma yb e
 
Ch ap t e r3 ) a n dt h e de t a i lsof ca ll/ re t u
rn . All 
of t h i sw i llbe u se fu li n
 
g e tt i ng
 
clon e( )
 
ab ove t ose t t h in g su p p rop e rly on t h e u se rst a ck 
of t h e
 
ch i ldt h re a d.
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
5
 
5
. R u nn in g Te st s
 

 
U se t h e f ollow in g scri p t fi lef orru n n i ng t h e te st s:
 
p r o m p t > . / t es t
-
t hr e a d
s .s h
 
-
c 
-
v
 
 

 
If y ou i mp lem e nt e dt h i ng s 
corre ct ly , y ou sh ou l dg e t some n ot i f i ca t i on t h a t t h e t e st s 

 
 

 
Th e  t e st s a ssu me  t h at  xv6  sou rce  code  i s f ou n d i n  t h e
 
s rc/
 
 
t h e re , t he scri p t w i llcomp lai n .
 
 

 
Th e  t e st  scri pt  doe s a  on e
-
ti me  clean  bu i ld of  y ou r xv6  
sou rce  code
 
u si n g  a  n ew ly 
g e n e rat e dma k e fi leca lle d
 
Mak ef ile. t est
. You ca n u se t h i sw h e n de b ug g in g ( a ssu mi ng 
y ou e ve rma k e mi st ak e s, t h at i s) , e. g. :
 
 
p r o m p t > c d  sr c /
 
p r o m p t > m a k e 
-
f  Ma k e f i le . t e st  q e mu
-
nox
 
 

 
You  ca n  su pp re ss t h e  rep e at e d bu i ldi ng  of  xv6  i n  t
 
-

 
f lag .  Th i s 
sh ou ldma k e re p ea t e dt e st i ng f a st e r:
 
p r o m p t > . / t es t
-
t hr e a d s
.s h  
-
s
 
 

 
You ca n sp e ci fi ca lly ru n a sin g lete st u sin g t hef ollow i n g comm a n d
 
 
./test
-
m m a p .s h  
-
c 
-
t 
7
 
-
v
 
Th e sp e ci fi ca lly ru n st he t e st _7 . ca lon e.
 
 

 
Th e ot h e ru su a lt e st i ng f lag sa re a lsoa va i lab le.S e e
 
t h e t e st i ng  
a pp e n di x 
f or 
more  
de t a i ls.
 
 

 
Th i sp roj e ct w i llha ve f ew h i dde n te st ca se sapa rt f rom th e p rovi de dt e st ca se s.
 
 
 
 
 
 
 
 
6
 
6
. 
Addi n g t e st fi l e sin si de xv6
 

 
In orde r
 
t o ru n  t h e  t e st  f i les i n si de xv6 , man u ally  cop y  th e  te st  fi les(
t es t _1. c,  t est _2. c
 

/
srcdi re ct ory . ( p re f e ra b ly i n a di f f e re n t na me lik e  
xv 6t es t _1. c

 

 
Ma k e t h e ne ce ssa ry ch an g e st ot he Ma k ef i le
 
UPROGS=
\
 
    
_ c at
\
 
    
_ e ch o
\
 
    
_ f or k t e st
\
 
    
_
x v6
t e s t_ 1
\
 
    
_ g re p
\
 
    
_ i ni t
\
 
    
_ k il l
\
 
    
_ln
\
 
    
_ls
\
 
    
_ m kd i r
\
 
    
_rm
\
 
    
_sh
\
 
    
_ s tr e s s fs
\
 
    
_ u se r t e st s
\
 
    
_wc
\
 
    
_ z om b i e
\
 
 
 
EXTRA=
\
 
    
m k fs . c  
xv 6
t e st _ 1
.c
 
u l i b .c  u s er .h  c a t . c e c h o. c  f or k t e s t. c  
g r e p . c k i l l .c
\
 
    
l n .c  l s .c  
m k di r . c  rm . c  st r e s sf s. c  u s e rt e s t s. c  w c. c  z o mb i e . c
\
 
    
p r in t f
. c u m a ll o c . c
\
 
    
R E AD M E  do t
-
b oc h s r c * . p l t o c . * ru n o f f  ru n o f f1  r u no f f .
l is t
\
 
    
. g db i n i t. t m p l g d b u ti l
\
 
 

 

m ak e clean &&m ak e qem u
-
n o x

. 
 
 
p r o m p t > m a k e 
c l e an  &
&  ma k e
 
qe m u
-
no x
 

 
On ce  i t  ha s compi led su cce ssf u lly  an d y ou a re i n si de xv6 p romp t , y ou  can  ru n t h e 
t e st .
 
$ 
 
$  x v 6 t es t
 
 
 
 
7
 

 
You ca n a lsoa ddy ou row n t e st ca se st ot e st you rsolu t i on e xt e n si ve ly .
 

 
On ce  y ou  a re  i n si de  xv6  qem u
 
p romp t  in  y our t e rmi n a l,  i f  y ou  w i sh  t o sh ut dow n 
xv6 a n de xi t qemu u se t h e f ollow in g k e y comb i na t i on s:
 

 
p re ssCt rl
-
A, t h e n re lea se y ou rk e y s, t h en p re ssX. ( Not Ct rl
-
X)
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
8
 
Appen di x 

 
Te st o pti o n s
 
 
Th e
 
run
-
te s ts .s h
 
s crip tis ca l l e dby va riou s te ste rstodoth e work ofte s tin g .Ea ch  
te s t is  a ctu a l ly  fa irl y  s im p le : it is  a  com p a ris on  of s ta n da rd ou tp u t a n d s ta n da rd 
e rror, a s  p e r the  p rog ra m s p e cifica tion .
 
 
I n  a ny  g iven  p rog ra m  
sp e cifica tion  directory , th e re e xis ts  a  
s p e cific
 
te s ts /
 
directory  which  h ol ds  th e  e xpe cte d return code , s ta n da rd ou tp u t, 
a n d s ta n da rd e rror in  fil e s  ca ll e d
 
n .rc,
 
n .out, a n d
 
n .e rr
 
(res p e ctive ly ) for e a ch  
te s t
 
n . Th e  te s tin g  fram e work  jus t s ta rts  a t
 
1
 
a n d k e
e p s  in crem e n tin g te s ts  u n til 
it ca n ' t fin d a n y  m ore or e n cou n te rs a  fa il u re . Th u s , a ddin g  n e w te s ts  is  e a sy ; 
jus t a dd th e  rel e van t fil e s  to th e  te s ts director y  a t th e l owe s ta va ila bl e  nu mb e r.
 
 
Th e  fil e s  n ee de d to de s crib e a  te s t n um b e r
 
n
 
a re:
 

 
n .rc: Th e  ret
u rn code  th e  p rog ra m  sh ou l d retu rn (u s u al l y 0  or1 )
 

 
n .ou t: Th e  s ta n da rd ou tp u t e xpe cte d from  th e  te s t
 

 
n .e rr: Th e  s ta n da rd e rror e xp e cte d from  th e  te s t
 

 
n .run : How to run  th e  te s t (which  a rg u m en ts it n e e ds , e tc.)
 

 
n .de s c: A  s h ort te xt de s crip tion  of th e  te s t
 

 
n .p re
 
(op tion a l ): C ode  to run  b e fore th e te s t,to s e t s om e th in g u p
 

 
n .p os t
 
(op tion a l ): C ode  to run  a fte r th e  te s t, to cl e a n s om e th in g u p
 
 
Th e re is al s o a  s in gl e  file  ca l le d
 
p re
 
which g e ts  run  on ce a t th e be g in n ing of 
te s tin g ;th is isofte n u se dtodoam ore
 
comp l exb u il dofa code b as e ,fore xa m pl e . 
To p reve n t repe a te d tim e
-
wa s ting  p re
-
te s t a ctivity , s u p p ress  th is  with  the
 
-
s
 
fl ag 
(a s  de s crib e d be l ow).
 
 
I n  m os t ca s e s , a  wra p pe r s crip t is  u s e d to ca l l
 
r un
-
tes ts .s h
 
to do th e n e ce ss a ry 
work .
 
 
Th e  op tion s  for
 
ru
n
-
te s ts .s h
 
in cl u de :
 

 
-
h
 
(th e  h e lp  me s sa g e )
 

 
-
v
 
(ve rb os e : p rin t wha t ea ch  te s t is  doin g )
 

 
-
t n
 
(run  on l y te s t
 
n)
 

 
-
c
 
(con tin u e  e ve n a fte r a  te s t fa il s )
 

 
-
d
 
(run  te s ts  n ot from
 
te s ts /
 
directory  b u t from  th is  directory  in s te a d)
 

 
-
s
 
(s u p p res s  runn in g  the  
on e
-
time  s e t of comm a n ds  in
 
p re
 
file )
 
 
 
