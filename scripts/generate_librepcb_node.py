from pathlib import Path
import math,uuid,json,csv,shutil,re
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'hardware/librepcb/marketsys-node'
NS=uuid.UUID('80e53dbc-cb56-4ec7-a306-607991aaac65')
def uid(s):return str(uuid.UUID(bytes=uuid.uuid5(NS,s).bytes,version=4))
def q(s):return json.dumps(str(s),ensure_ascii=True)
def f(v):return str(round(float(v),5))
def write(p,s):
 p.parent.mkdir(parents=True,exist_ok=True);p.write_text(s+'\n')
def text(key,value,x,y,size=1.8,layer='sch_documentation',align='left bottom',rotation=0):
 return f'(text {uid(key)} (layer {layer}) (height {f(size)}) (align {align}) (position {f(x)} {f(y)}) (rotation {f(rotation)}) (lock false) (value {q(value)}))'
def poly(key,points,width=.25,fill=False,grab=False):
 return f'(polygon {uid(key)} (layer sym_outlines) (width {f(width)}) (fill {str(fill).lower()}) (grab_area {str(grab).lower()}) '+''.join(f'(vertex (position {f(x)} {f(y)}) (angle 0.0))' for x,y in points)+')'
def meta(kind,name):
 return f'(name {q(name)}) (description {q(name+"; schematic model, no PCB footprint assigned")}) (keywords "marketsys,node") (author "MarketSys") (version "0.1") (created 2026-11-04T00:00:00Z) (deprecated false) (generated_by "marketsys-node-generator")'
GRAPHICS={};LIB={};PINS={};SYMS={};COMPS={};SIGNALS={};INST=[];NETS={};SEGS=[];NOTES=[];BOM=[];CONNECTIONS={};counter=0

def symbol(kind,pins,graphics):
 sid=uid('sym/'+kind);PINS[kind]={};GRAPHICS[kind]=graphics
 out=[f'(librepcb_symbol {sid}',meta('sym',kind),'(grid_interval 2.54)']
 for name,x,y,rot,display in pins:
  pid=uid('pin/'+kind+'/'+name);PINS[kind][name]=dict(uuid=pid,x=x,y=y,rotation=rot)
  out.append(f'(pin {pid} (name {q(re.sub(r"[^A-Za-z0-9_+-]", "_", display))}) (position {f(x)} {f(y)}) (rotation {f(rot)}) (length 2.54) (name_position 3.4 0.0) (name_rotation 0.0) (name_height 1.6) (name_align left center))')
 out+=graphics
 write(OUT/f'library/sym/{sid}/.librepcb-sym','2');write(OUT/f'library/sym/{sid}/symbol.lp','\n'.join(out)+')');SYMS[kind]=sid

def component(kind,name,gates,required=True):
 cid=uid('cmp/'+kind);variant=uid('variant/'+kind);SIGNALS[kind]={};gs={}
 out=[f'(librepcb_component {cid}',meta('cmp',name),'(schematic_only false)',f'(default_value {q(name)}) (prefix "U")']
 for gate,sym,pairs in gates:
  for pin,signal in pairs.items():
   if signal in SIGNALS[kind]:continue
   sig=uid('signal/'+kind+'/'+signal);SIGNALS[kind][signal]=sig
   out.append(f'(signal {sig} (name {q(signal)}) (role passive) (required {str(required).lower()}) (negated false) (clock false) (forced_net ""))')
 out.append(f'(variant {variant} (norm "") (name "default") (description "")')
 for gate,sym,pairs in gates:
  gid=uid('gate/'+kind+'/'+gate);gs[gate]=dict(uuid=gid,sym=sym,pairs=pairs)
  out.append(f'(gate {gid} (symbol {SYMS[sym]}) (position 0.0 0.0) (rotation 0.0) (required true) (suffix {q(gate if len(gates)>1 else "")})')
  for pin,signal in pairs.items():out.append(f'(pin {PINS[sym][pin]["uuid"]} (signal {SIGNALS[kind][signal]}) (text none))')
  out.append(')')
 out.append('))');write(OUT/f'library/cmp/{cid}/.librepcb-cmp','2');write(OUT/f'library/cmp/{cid}/component.lp','\n'.join(out));LIB[kind]=dict(uuid=cid,variant=variant,gates=gs)

def boxsym(kind,pins,w,h):
 graphics=[poly(kind+'box',[(-w,-h),(w,-h),(w,h),(-w,h),(-w,-h)],grab=True)]
 for name,x,y,rot,display in pins:
  # Independent horizontal names at the inner ends of both sides.
  graphics.append(text(kind+'label'+name,display,x+(3.4 if x<0 else -3.4),y,1.6,'sym_outlines','left center' if x<0 else 'right center'))
 symbol(kind,pins,graphics)

# All module signal names are functional board/header identifiers, not guessed bare-SiP pad numbers.
modulepins=[('3V3',-27.94,35.56,0,'3V3'),('GND',-27.94,25.4,0,'GND'),('SWDIO',-27.94,10.16,0,'PA13 / SWDIO'),('SWCLK',-27.94,0,0,'PA14 / SWCLK'),('UART_TX',-27.94,-10.16,0,'PA2 / UART TX'),('UART_RX',-27.94,-20.32,0,'PA3 / UART RX'),('BLUE',27.94,35.56,180,'PB3 / J8.2'),('RED',27.94,0,180,'PB4 / J8.3'),('GREEN',27.94,-35.56,180,'PB5 / J8.4')]
boxsym('RAK3272-used',modulepins,25.4,43.18);component('RAK','RAK3272-SiP breakout (used signals)',[('', 'RAK3272-used',{p[0]:p[0] for p in modulepins})])
driverpins=[('IN+',-20.32,10.16,0,'IN+'),('EN',-20.32,0,0,'EN'),('IN-',-20.32,-10.16,0,'IN-'),('L+',20.32,5.08,180,'L+'),('L-',20.32,-5.08,180,'L-')]
boxsym('LD1500SB',driverpins,17.78,15.24);component('DRIVER','LD1500SB module',[('', 'LD1500SB',{p[0]:p[0] for p in driverpins})])
# Resistors drawn in IEC style.
symbol('Resistor',[('1',-5.08,0,0,'1'),('2',5.08,0,180,'2')],[poly('resistorbody',[(-2.54,-1.016),(-2.54,1.016),(2.54,1.016),(2.54,-1.016),(-2.54,-1.016)],grab=True)])
component('R','Resistor',[('', 'Resistor',{'1':'1','2':'2'})])
# N-channel enhancement MOSFET units, actual FDC6401N pin numbers.
for unit,g,s,d in [('A',1,5,6),('B',3,2,4)]:
 kind='NMOS-'+unit
 gfx=[poly(kind+'gate',[(-7.62,0),(-5.08,0)]),poly(kind+'gatebar',[(-5.08,-3.81),(-5.08,3.81)]),poly(kind+'drain',[(5.08,5.08),(5.08,2.54),(-2.54,2.54)]),poly(kind+'source',[(5.08,-5.08),(5.08,-2.54),(-2.54,-2.54)]),poly(kind+'bulk',[(-2.54,0),(2.54,0),(2.54,-2.54)]),poly(kind+'arrow',[(-2.54,0),(-.6,.8),(-.6,-.8),(-2.54,0)],fill=True)]
 for i,(a,b) in enumerate([(-3.81,-1.9),(-.95,.95),(1.9,3.81)]):gfx.append(poly(kind+'channel'+str(i),[(-2.54,a),(-2.54,b)]))
 gfx += [text(kind+'gl',f'G {g}',-9,1.4,1.5,'sym_outlines'),text(kind+'dl',f'D {d}',7,5,1.5,'sym_outlines'),text(kind+'sl',f'S {s}',7,-5.4,1.5,'sym_outlines')]
 symbol(kind,[('G',-10.16,0,0,str(g)),('D',5.08,7.62,270,str(d)),('S',5.08,-7.62,90,str(s))],gfx)
component('FDC','FDC6401N',[('A','NMOS-A',{'G':'G1','D':'D1','S':'S1'}),('B','NMOS-B',{'G':'G2','D':'D2','S':'S2'})],required=False)
# One RGB part, three schematic gates. Functional LED terminals intentionally have no guessed package pad numbers.
for colour in ('BLUE','RED','GREEN'):
 kind='LED-'+colour
 gfx=[poly(kind+'axis',[(-7.62,0),(7.62,0)]),poly(kind+'triangle',[(-2.54,0),(2.54,2.54),(2.54,-2.54),(-2.54,0)],grab=True),poly(kind+'bar',[(-2.54,-2.54),(-2.54,2.54)])]
 for k,x in enumerate([0,3]):
  gfx.extend([poly(kind+'ray'+str(k),[(x,3.5),(x+2.3,5.8)]),poly(kind+'raytip'+str(k),[(x+1,5.8),(x+2.3,5.8),(x+2.3,4.5)])])
 gfx.extend([text(kind+'kl','K (-)',-8,-3.6,1.4,'sym_outlines'),text(kind+'al','A (+)',5,-3.6,1.4,'sym_outlines')])
 symbol(kind,[('K',-10.16,0,0,'K'),('A',10.16,0,180,'A')],gfx)
component('RGB','3537 RGB LED, separate die terminals',[(c,'LED-'+c,{'A':c+'_A','K':c+'_K'}) for c in ('BLUE','RED','GREEN')])
for kind,pinnames in [('POWER',['5V','GND','3V3']),('DAP',['3V3','GND','SWDIO','SWCLK','DAP_TX','DAP_RX'])]:
 pins=[(p,15.24,(len(pinnames)-1)*2.54-i*5.08,180,p) for i,p in enumerate(pinnames)]
 boxsym(kind,pins,12.7,len(pinnames)*2.54+1.27);component(kind,('Regulated bench power' if kind=='POWER' else 'Programming interface'),[('',kind,{p:p for p in pinnames})])

def net(name):
 if name not in NETS:NETS[name]=uid('net/'+name)
 return NETS[name]
def add(ref,kind,value,mapping):
 COMPS[ref]=dict(uuid=uid('instance/'+ref),kind=kind,value=value,mapping=mapping)
 for signal in SIGNALS[kind]:
  name=mapping.get(signal);CONNECTIONS[ref+'.'+signal]=name
  if name:net(name)
 BOM.append((ref,value))
 return ref

def place(ref,gate,x,y,rotation=0,label=None):
 global counter
 c=COMPS[ref];g=LIB[c['kind']]['gates'][gate];sid=uid('placed/'+ref+'/'+gate);sym=g['sym'];a=math.radians(rotation)
 pinmap={}
 for p,info in PINS[sym].items():
  px=x+info['x']*math.cos(a)-info['y']*math.sin(a);py=y+info['x']*math.sin(a)+info['y']*math.cos(a)
  pinmap[p]=dict(uuid=info['uuid'],x=round(px,5),y=round(py,5),symbol=sid,signal=g['pairs'][p],ref=ref)
 inst=f'(symbol {sid} (component {c["uuid"]}) (lib_gate {g["uuid"]}) (position {f(x)} {f(y)}) (rotation {f(rotation)}) (mirror false)'
 if c['kind']=='R':
  if rotation==0:labels=[(ref,x,y+2,'center bottom'),(value_or(c),x,y-2,'center top')]
  else:labels=[(ref,x-2.2,y+1,'right bottom'),(value_or(c),x-2.2,y-1,'right top')]
 elif c['kind']=='FDC':labels=[(label or (ref+gate),x-9,y+9.7,'left bottom'),('FDC6401N',x-9,y-10,'left top')]
 elif c['kind']=='RGB':labels=[('D1 '+gate,x,y+7,'center bottom')]
 elif c['kind']=='RAK':labels=[('U1  RAK3272-SiP',x,y+45,'center bottom'),('Breakout: used signals only',x,y-46,'center top')]
 elif c['kind']=='DRIVER':labels=[('U2  LD1500SB',x,y+17,'center bottom'),('EN tied to IN+',x,y-18,'center top')]
 else:labels=[(ref+'  '+c['value'],x-12.7,y+len(g['pairs'])*2.54+3,'left bottom')]
 for i,(v,tx,ty,align) in enumerate(labels):inst+=text('insttext/'+ref+'/'+gate+'/'+str(i),v,tx,ty,1.7,'sym_names' if i==0 else 'sym_values',align)
 for i,graphic in enumerate(GRAPHICS[sym]):
  if not graphic.startswith('(text '):continue
  m=re.search(r'\(position ([-0-9.]+) ([-0-9.]+)\)',graphic)
  gx,gy=map(float,m.groups());tx=x+gx*math.cos(a)-gy*math.sin(a);ty=y+gx*math.sin(a)+gy*math.cos(a)
  graphic=re.sub(r'^\(text [^ ]+', '(text '+uid('graphic/'+ref+'/'+gate+'/'+str(i)),graphic)
  graphic=graphic.replace('(layer sym_outlines)','(layer sym_values)')
  graphic=re.sub(r'\(position [-0-9.]+ [-0-9.]+\)',f'(position {f(tx)} {f(ty)})',graphic)
  inst+=graphic
 INST.append(inst+')');return pinmap

def value_or(c):return c['value']
def endpoint(p):return f'(symbol {p["symbol"]}) (pin {p["uuid"]})'
def segment(name,edges,label=None):
 global counter
 counter+=1;key='segment/'+str(counter);parts=[f'(netsegment {uid(key)} (net {net(name)})'];junctions={}
 def ep(p):
  if isinstance(p,dict):
   actual=COMPS[p['ref']]['mapping'][p['signal']]
   assert actual==name,(p,name,actual)
   return endpoint(p)
  t=tuple(p)
  if t not in junctions:junctions[t]=uid(key+'/junction/'+str(t));parts.append(f'(junction {junctions[t]} (position {f(t[0])} {f(t[1])}))')
  return f'(junction {junctions[t]})'
 lines=[]
 for i,(a,b) in enumerate(edges):
  ea,eb=ep(a),ep(b);lines.append(f'(line {uid(key+"/line/"+str(i))} (width 0.2) (from {ea}) (to {eb}))')
 parts+=lines
 if label:parts.append(f'(label {uid(key+"/label")} (position {f(label[0])} {f(label[1])}) (rotation 0.0) (mirror false))')
 SEGS.append(''.join(parts)+')')
def stub(pin,name,dx=-10.16):
 end=(pin['x']+dx,pin['y']);segment(name,[(pin,end)],label=end)

add('U1','RAK','RAK3272-SiP breakout',dict(zip([p[0] for p in modulepins],['V3V3','GND','SWDIO','SWCLK','UART_TX','UART_RX','PB3_BLUE','PB4_RED','PB5_GREEN'])))
add('U2','DRIVER','LD1500SB',{'IN+':'V5','EN':'V5','IN-':'GND','L+':'LED_PLUS','L-':'LED_RETURN'})
add('IC1','FDC','FDC6401N',{'G1':'GATE_BLUE','D1':'LED_BLUE_K','S1':'LED_RETURN','G2':'GATE_RED','D2':'LED_RED_K','S2':'LED_RETURN'})
add('IC2','FDC','FDC6401N',{'G1':'GATE_GREEN','D1':'LED_GREEN_K','S1':'LED_RETURN','G2':None,'D2':None,'S2':None})
add('D1','RGB','3537 RGB LED',{c+'_A':'LED_PLUS' for c in ('BLUE','RED','GREEN')}|{c+'_K':'LED_'+c+'_K' for c in ('BLUE','RED','GREEN')})
add('J1','POWER','REGULATED INPUT',{'5V':'V5','GND':'GND','3V3':'V3V3'})
add('J2','DAP','RAKDAP1 interface',{'3V3':'V3V3','GND':'GND','SWDIO':'SWDIO','SWCLK':'SWCLK','DAP_TX':'UART_RX','DAP_RX':'UART_TX'})
u=place('U1','',55.88,105.0);drv=place('U2','',220,195)
power=place('J1','',35,195);dap=place('J2','',50,27)
for key in ('3V3','GND','SWDIO','SWCLK','UART_TX','UART_RX'):stub(u[key],COMPS['U1']['mapping'][key],-12.7)
for pin in power.values():stub(pin,COMPS['J1']['mapping'][pin['signal']],12.7)
for pin in dap.values():stub(pin,COMPS['J2']['mapping'][pin['signal']],12.7)
# Explicit EN jumper to the driver input positive.
x=drv['IN+']['x']-10.16
segment('V5',[(drv['IN+'],(x,drv['IN+']['y'])),((x,drv['IN+']['y']),(x,drv['EN']['y'])),((x,drv['EN']['y']),drv['EN'])],label=(x,drv['IN+']['y']))
stub(drv['IN-'],'GND',-10.16);stub(drv['L-'],'LED_RETURN',12.7)
plus_edges=[(drv['L+'],(265,drv['L+']['y']))];last=(265,drv['L+']['y'])
for i,(colour,row,ref,gate) in enumerate([('BLUE',140.56,'IC1','A'),('RED',105,'IC1','B'),('GREEN',69.44,'IC2','A')]):
 rref='R'+str(i+1);pref='R'+str(i+4)
 add(rref,'R','1k',{'1':'PB'+str(i+3)+'_'+colour,'2':'GATE_'+colour})
 add(pref,'R','10k',{'1':'LED_RETURN','2':'GATE_'+colour})
 rr=place(rref,'',109.22,row);pd=place(pref,'',121.92,row-12.7,90)
 fet=place(ref,gate,147.32,row,label=ref+gate+'  '+colour)
 led=place('D1',colour,215.9,row)
 segment('PB'+str(i+3)+'_'+colour,[(u[colour],rr['1'])])
 gatepoint=(121.92,row)
 segment('GATE_'+colour,[(rr['2'],gatepoint),(gatepoint,fet['G']),(gatepoint,pd['2'])])
 return_y=row-22.86;sourcepoint=(fet['S']['x'],return_y);leftpoint=(pd['1']['x'],return_y);rightpoint=(170.18,return_y)
 segment('LED_RETURN',[(pd['1'],leftpoint),(leftpoint,sourcepoint),(fet['S'],sourcepoint),(sourcepoint,rightpoint)],label=rightpoint)
 bend1=(190.5,fet['D']['y']);bend2=(190.5,row)
 segment('LED_'+colour+'_K',[(fet['D'],bend1),(bend1,bend2),(bend2,led['K'])])
 branch=(265,row);plus_edges.extend([(last,branch),(branch,led['A'])]);last=branch
place('IC2','B',240,27,label='IC2B  SPARE / NC')
segment('LED_PLUS',plus_edges,label=(265,drv['L+']['y']))
# Netlist checks independent of drawing coordinates.
assert CONNECTIONS['U2.IN+']==CONNECTIONS['U2.EN']=='V5'
assert CONNECTIONS['U2.L-']=='LED_RETURN' and CONNECTIONS['U2.IN-']=='GND'
for name in ('IC1.S1','IC1.S2','IC2.S1'):assert CONNECTIONS[name]=='LED_RETURN'
assert CONNECTIONS['IC1.D1']=='LED_BLUE_K' and CONNECTIONS['IC1.D2']=='LED_RED_K' and CONNECTIONS['IC2.D1']=='LED_GREEN_K'
assert CONNECTIONS['J2.DAP_TX']==CONNECTIONS['U1.UART_RX']=='UART_RX'
assert CONNECTIONS['J2.DAP_RX']==CONNECTIONS['U1.UART_TX']=='UART_TX'

NOTES=[text('title','MARKETSYS  |  STANDARD PEN INDICATOR NODE',10,228,4),text('subtitle','Bench schematic - original MOSFET allocation - editable LibrePCB project',10,220,2.1),text('powernote','J1: externally regulated 5 V + 3.3 V. No battery charger or regulator is included.',10,210,1.8),text('currentnote','Trim driver to approximately 160 mA. Firmware selects one colour at a time.',175,165,1.8),text('returnnote','LED_RETURN = driver L-; keep separate from supply GND.',175,157,1.8),text('programnote','Programming power is an ALTERNATIVE 3.3 V source; do not parallel supplies.',10,5,1.7),text('unusednote','IC2B: unused in the standard assembly. All three terminals unconnected.',190,5,1.7),text('scopenote','Module connections shown functionally. No PCB footprints or manufacturing pad assignments.',10,-5,1.7),text('variantnote','Node2 repair only: red uses IC2B (G3/S2/D4) instead of IC1B. This sheet preserves standard wiring.',10,-12,1.7),text('antenna','Attach the module antenna before powering the node.',10,52,1.7)]
# Write complete self-contained native project.
write(OUT/'.librepcb-project','2');write(OUT/'marketsys-node.lpp','LIBREPCB-PROJECT')
write(OUT/'project/metadata.lp',f'(librepcb_project_metadata {uid("project")} (name "MarketSys Node - Standard") (author "MarketSys") (version "0.1") (created 2026-11-04T00:00:00Z))')
write(OUT/'project/settings.lp','(librepcb_project_settings (library_locale_order (locale "en_US")) (library_norm_order (norm "IEC 60617")) (custom_bom_attributes) (default_lock_component_assembly false))')
write(OUT/'project/jobs.lp','(librepcb_jobs)');write(OUT/'boards/boards.lp','(librepcb_boards)');write(OUT/'circuit/erc.lp','(librepcb_erc)')
variant=uid('assembly');nc=uid('netclass');circuit=[f'(librepcb_circuit (variant {variant} (name "Standard") (description "Original standard node"))',f'(netclass {nc} (name "default") (default_trace_width inherit) (default_via_drill_diameter inherit) (min_copper_copper_clearance 0.0) (min_copper_width 0.0) (min_via_drill_diameter 0.0))']
for name,nid in NETS.items():circuit.append(f'(net {nid} (auto false) (name {q(name)}) (netclass {nc}))')
for ref,c in COMPS.items():
 l=LIB[c['kind']];parts=[f'(component {c["uuid"]} (lib_component {l["uuid"]}) (lib_variant {l["variant"]}) (name {q(ref)}) (value {q(c["value"])}) (lock_assembly false)']
 for signal,sid in SIGNALS[c['kind']].items():parts.append(f'(signal {sid} (net {net(c["mapping"][signal]) if c["mapping"].get(signal) else "none"}))')
 circuit.append(''.join(parts)+')')
write(OUT/'circuit/circuit.lp','\n'.join(circuit)+')')
write(OUT/'schematics/schematics.lp','(librepcb_schematics (schematic "schematics/node/schematic.lp"))')
write(OUT/'schematics/node/schematic.lp',f'(librepcb_schematic {uid("schematic")} (name "Node - Standard") (grid (interval 2.54) (unit millimeters))\n'+'\n'.join(INST+SEGS+NOTES)+')')
font=OUT/'resources/fontobene/newstroke.bene'
assert font.exists(), 'Preserve the bundled CC0 newstroke.bene font when regenerating'
write(OUT/'.gitignore','.autosave/\n.backup/\nuser/\n*.user.lp\n.lock\n*~')
with (OUT/'connections.csv').open('w',newline='') as out:
 writer=csv.writer(out);writer.writerow(['component.signal','net']);writer.writerows(sorted(CONNECTIONS.items()))
write(OUT/'README.md','''# MarketSys standard node schematic

Open marketsys-node.lpp in LibrePCB 2.x, or open the accompanying .lppz archive.
All schematic symbols/components are embedded; no external library installation
is needed. This is an editable electrical schematic, not a PCB layout.

The original standard allocation is preserved: IC1A blue (G1/S5/D6), IC1B red
(G3/S2/D4), IC2A green (G1/S5/D6); IC2B unused. Node2's repair moved red to IC2B;
that repair is documented separately and is not imposed on every node here.

PB3=blue, PB4=red, PB5=green, each through 1k. Each gate has a 10k pull-down to
its own source/driver L-. LED positives join driver L+; negatives join the
respective drains. LED_RETURN/driver L- is distinct from supply GND. EN is tied
to IN+. Inputs are externally regulated 5 V for the driver and 3.3 V for RAK.
J2 is a logical six-wire programming interface, not a claim about the physical
pin order of a particular programmer connector. Do not parallel its 3.3 V feed
with another powered source. Attach the antenna before powering the module.

RAK is the existing breakout assembly, shown with its used functional signals;
it is not a bare-module land pattern. RGB die terminals are functional labels;
actual LED package pad numbering is unverified. No footprints, devices or board
are assigned. Verify module/header dimensions and LED pads before PCB layout.

The schematic does not add an unselected battery, charger, regulator or EN GPIO.
Firmware state colours and the unresolved intermittent startup observation do
not change these nets. connections.csv provides the explicit connectivity list.

Sources:
- Project docs/indicator-bench-wiring.md (original allocation plus Node2 repair).
- https://www.onsemi.com/download/data-sheet/pdf/fdc6401n-d.pdf
- https://docs.rakwireless.com/product-categories/wisduo/rak3272-sip-breakout-board/datasheet/
- https://eletechsup.com/products/adjustable-28-1500ma-output-current-high-brightness-led-constant-current-driver-board-for-flashlight-headlights-emergency-lights

Generated with native format references from LibrePCB/librepcb-example-projects.
Bundled NewStroke/FontoBene stroke font is CC0-1.0, as noted in its file header.
''')
print(OUT)
print('Components:',len(COMPS),'nets:',len(NETS),'symbol instances:',len(INST))
