"""Draw the verified MarketSys node wiring as a high-resolution PNG (Pillow)."""
from pathlib import Path
import math
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'docs/images/node-complete-wiring.png'
OUT.parent.mkdir(parents=True,exist_ok=True)
W,H=3800,2960
im=Image.new('RGB',(W,H),'#ffffff');d=ImageDraw.Draw(im)
fontroot=Path('C:/Windows/Fonts')
if fontroot.exists(): regular=fontroot/'arial.ttf';bold=fontroot/'arialbd.ttf'
else:
 fontroot=Path('/usr/share/fonts/truetype/dejavu');regular=fontroot/'DejaVuSans.ttf';bold=fontroot/'DejaVuSans-Bold.ttf'
def f(size=38,b=False):return ImageFont.truetype(str(bold if b else regular),size)
INK='#18364b';MUTED='#536a79';BLUE='#2462cf';RED='#c6363c';GREEN='#16834b';ORANGE='#c36b16';GOLD='#b18a00';TEAL='#007e90';GND='#34444d'
def txt(x,y,t,size=38,b=False,c=INK,anchor=None):d.text((x,y),t,font=f(size,b),fill=c,anchor=anchor)
def wire(points,c=INK,w=8):d.line(points,fill=c,width=w,joint='curve')
def dot(x,y,c=INK):d.ellipse((x-11,y-11,x+11,y+11),fill=c)
def box(bounds,fill='#f5f8fb',outline='#a9bbc7',r=24,w=4):d.rounded_rectangle(bounds,radius=r,fill=fill,outline=outline,width=w)
def res_h(x1,x2,y,c,label):
 wire([(x1,y),(x1+25,y)],c);step=(x2-x1-50)/10
 p=[(x1+25,y)]+[(x1+25+i*step,y+(-17 if i%2 else 17)) for i in range(1,10)]+[(x2-25,y)]
 wire(p,c,6);wire([(x2-25,y),(x2,y)],c);txt((x1+x2)/2,y-45,label,36,True,c,'ms')
def res_v(x,y1,y2):
 wire([(x,y1),(x,y1+22)],'#647783',6);step=(y2-y1-44)/8
 points=[(x,y1+22)]+[(x+(-14 if i%2 else 14),y1+22+i*step) for i in range(1,8)]+[(x,y2-22)]
 wire(points,'#647783',6);wire([(x,y2-22),(x,y2)],'#647783',6)
def terminal(x,y,c):d.ellipse((x-10,y-10,x+10,y+10),fill='white',outline=c,width=5)
def jump_return(y):
 wire([(2400,y),(3382,y)],ORANGE);wire([(3418,y),(3550,y)],ORANGE)
 d.arc((3382,y-18,3418,y+18),180,360,fill=ORANGE,width=8);dot(3550,y,ORANGE)

# Header
box((0,0,W,208),'#18364b','#18364b',0)
txt(130,55,'MarketSys | Complete Node Wiring',82,True,'white')
txt(135,148,'RAK3272-SiP / RAK3172-SiP  +  LD1500SB  +  FDC6401N  +  one RGB LED',40,False,'#d7e8f3')
txt(130,248,'BENCH OPERATING CONNECTIONS',40,True)
txt(3670,248,'Revision 0.4.2  |  October 26, 2026',34,False,MUTED,'ra')

# Power and driver
box((170,315,1000,720),'#eef6f8')
txt(220,355,'REGULATED BENCH SUPPLY',42,True)
txt(220,408,'5 V rail',39,True,RED)
txt(220,530,'Common GND',39,True,GND)
txt(220,640,'3.3 V rail',39,True,TEAL)
wire([(1000,420),(1500,420)],RED)
wire([(1000,540),(1500,540)],GND)
wire([(450,720),(450,850)],TEAL)
wire([(1250,540),(1250,780),(800,780),(800,850)],GND);dot(1250,540,GND)
terminal(1000,420,RED);terminal(1000,540,GND);terminal(450,720,TEAL)
box((1500,315,2400,620),'#fff8e8','#c1a76c')
txt(1950,355,'LD1500SB DRIVER',45,True,INK,'ma')
txt(1530,420,'IN+  (green lead)',35,False,INK,'lm')
txt(1530,540,'IN-  (red lead)',35,False,INK,'lm')
txt(2370,420,'L+  (yellow)',35,True,GOLD,'rm')
txt(2370,540,'L-  (orange)',35,True,ORANGE,'rm')
wire([(1700,620),(1700,675)],'#8a5d39',6);terminal(1700,675,'#8a5d39')
txt(1750,675,'EN (brown): UNCONNECTED',34,True,'#8a5d39','lm')
wire([(2400,420),(3400,420),(3400,1750)],GOLD)
wire([(3550,540),(3550,2020)],ORANGE);jump_return(540)
txt(3400,355,'L+',39,True,GOLD,'ma');txt(3550,475,'L-',39,True,ORANGE,'ma')
txt(1940,735,'Current trim: approximately 160 mA; one color at a time',34,False,MUTED,'ma')

# Node board
box((220,850,1000,2110),'#eff4fb','#9bb3cd')
txt(270,900,'RAK3272-SiP NODE',42,True)
txt(490,815,'3V3',34,True,TEAL,'lm');txt(845,815,'GND',34,True,GND,'lm')
terminal(450,850,TEAL);terminal(800,850,GND)
txt(270,1160,'Power: 3.3 V only',38,True)
txt(270,1230,'GPIO HIGH = color ON',34)
txt(270,1630,'ANT: antenna attached',36,True)
txt(270,1700,'Address: build as node 1-50',32,False,MUTED)
txt(2890,710,'ONE RGB LED',41,True,INK,'ma')
txt(2890,760,'Three dies shown separately below',31,False,MUTED,'ma')

channels=[('BLUE','PB3','J8 position 2','IC1 / Q1','P1','P6','P5',BLUE,0),('RED','PB4','J8 position 3','IC1 / Q2','P3','P4','P2',RED,450),('GREEN','PB5','J8 position 4','IC2 / Q3','P1','P6','P5',GREEN,900)]
for name,pb,header,ic,g,dr,src,c,dy in channels:
 gy=1000+dy;ly=850+dy;sy=1120+dy
 # Gate trace and its series resistor
 txt(275,gy-20,pb,56,True,c);txt(275,gy+40,header,30,False,MUTED)
 terminal(1000,gy,c)
 wire([(1000,gy),(1480,gy)],c);res_h(1480,1710,gy,c,'1 kOhm');wire([(1710,gy),(2270,gy)],c)
 # Pulldown is gate-to-source, not supply GND
 dot(2070,gy,c);wire([(2070,gy),(2070,gy+20)],c,6);res_v(2070,gy+20,sy-20)
 wire([(2070,sy-20),(2070,sy),(2400,sy)],ORANGE,6)
 txt(2010,gy+70,'10 kOhm',34,True,INK,'rm');txt(2010,gy+111,'G to S',28,False,MUTED,'rm')
 # Three-pin MOSFET block, with precise package pin numbers
 box((2270,gy-70,2540,gy+70),'#ffffff',c,15,5)
 txt(2580,gy-10,ic,38,True,c,'lm')
 txt(2290,gy-5,'G '+g,31,True,INK,'lm')
 txt(2465,gy-51,'D '+dr,29,True,INK,'mm')
 txt(2465,gy+48,'S '+src,29,True,INK,'mm')
 wire([(2400,ly),(2400,gy-70)],c)
 wire([(2400,gy+70),(2400,sy)],ORANGE);dot(2400,sy,ORANGE)
 # LED cathode to drain; all anodes join L+
 wire([(2400,ly),(2750,ly)],c)
 box((2750,ly-53,3100,ly+53),'#ffffff',c,18,5)
 d.ellipse((2790,ly-20,2830,ly+20),fill=c)
 txt(2860,ly,name+' DIE',33,True,c,'lm')
 txt(2715,ly-20,'K (-)',29,True,c,'rs');txt(3130,ly-20,'A (+)',29,True,GOLD,'ls')
 wire([(3100,ly),(3400,ly)],GOLD);dot(3400,ly,GOLD)
 # Shared source return jumps over L+ (no connection at crossing)
 if sy <= 1750: jump_return(sy)
 else:
  wire([(2400,sy),(3550,sy)],ORANGE);dot(3550,sy,ORANGE)

# Critical separation note beneath circuit
box((1390,2100,3650,2210),'#fff5e8','#dfbd7e',15,3)
txt(1430,2130,'ALL THREE SOURCES -> DRIVER L- (orange).',38,True)
txt(1430,2175,'L- is a separate driver return: do not jumper it to supply GND.',34,False)

# Bottom reference panels
box((130,2280,1850,2790),'#f5f8fb')
txt(175,2315,'FDC6401N CONNECTION CHECK',40,True)
txt(175,2378,'Channel',33,True);txt(600,2378,'Gate',33,True);txt(920,2378,'Source',33,True);txt(1260,2378,'Drain',33,True)
for yy,values,c in [(2440,('Blue / IC1','P1','P5','P6'),BLUE),(2505,('Red / IC1','P3','P2','P4'),RED),(2570,('Green / IC2','P1','P5','P6'),GREEN)]:
 for xx,v in zip([175,600,920,1260],values):txt(xx,yy,v,36,True,c)
txt(175,2640,'IC2 unused Q4: gate P3, source P2, drain P4.',33)
txt(175,2700,'No LED connection on Q4. Use actual package pin numbers;',31,False,MUTED)
txt(175,2740,'do not infer orientation from the adapter square pad.',31,False,MUTED)
box((1910,2280,3670,2790),'#f5f8fb')
txt(1955,2315,'PROGRAMMING ONLY | RAKDAP1',40,True)
prog=[('3V3 -> node 3V3','GND -> node GND'),('TMS/IO -> PA13 (SWDIO)','TCK/CK -> PA14 (SWCLK)'),('TX -> PA3 (UART RX)','RX -> PA2 (UART TX)')]
for yy,(a,b) in zip([2390,2455,2520],prog):txt(1955,yy,a,33);txt(2820,yy,b,33)
txt(1955,2600,'Unplug USB before moving wires; breadboard power OFF.',33,True)
txt(1955,2660,'Disconnect the breadboard 3.3 V feed while DAPLink powers',31)
txt(1955,2700,'the node. Never connect two 3.3 V supply outputs together.',31)
txt(1955,2750,'DAPLink 5 V and nRST are unused in this bench hookup.',31,False,MUTED)

# Footer
txt(130,2840,'Wire colors identify signals in this drawing; driver lead colors are written beside its terminals.',32,False,MUTED)
txt(130,2890,'Source: verified indicator-bench-wiring.md and hardware-identification.md | GPIO and pin map unchanged in firmware 0.4.2.',29,False,MUTED)
im.save(OUT,dpi=(300,300),optimize=True)
print(OUT, im.size)
