"""Three-node walkthrough driven by the controller destination-only bench input.
Uses controller business flow; the deferred application event interface is not implemented.
"""
import argparse,json,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
PYTHON=ROOT.parent/'Tools/marketsys-uart-venv/bin/python'
# Only destinations are sent. All resulting states are computed by the controller.
STEPS=[
 ("All OFF",None,["OFF","OFF","OFF"]),
 ("Destination P1",1,["ACTIVE","OFF","OFF"]),
 ("Destination P2",2,["LAST","ACTIVE","OFF"]),
 ("Destination P3",3,["OFF","LAST","ACTIVE"]),
 ("Destination P3 (repeat, HOLD_OPEN)",3,["OFF","OFF","HOLD_OPEN"]),
 ("Destination P2",2,["OFF","ACTIVE","LAST"]),
 ("Destination P1",1,["ACTIVE","LAST","OFF"]),
 ("Destination P3",3,["LAST","OFF","ACTIVE"]),
 ("Destination P2",2,["OFF","ACTIVE","LAST"]),
 ("Destination P2 (repeat, HOLD_OPEN)",2,["OFF","HOLD_OPEN","OFF"]),
 ("Destination P1",1,["ACTIVE","LAST","OFF"]),
 ("Destination P1 (repeat, HOLD_OPEN)",1,["HOLD_OPEN","OFF","OFF"]),
 ("Destination P2",2,["LAST","ACTIVE","OFF"]),
 ("Destination P3",3,["OFF","LAST","ACTIVE"]),
 ("Finish all OFF",None,["OFF","OFF","OFF"]),
]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('action',choices=['plan','start','next','stop','status','auto'])
parser.add_argument('--interval', type=int, default=15, help='Seconds to hold each verified step in auto mode')
parser.add_argument('--passes', type=int, default=4, help='Number of complete automatic passes')
args=parser.parse_args()
if args.interval < 1 or args.passes < 1:
 parser.error('--interval and --passes must be positive')
statefile=ROOT/'build/flow-walkthrough.json'
state=json.loads(statefile.read_text()) if statefile.exists() else {'step':-1}
def radio(action,node):
 p=subprocess.run([str(PYTHON),str(ROOT/'scripts/radio_link.py'),action,'--node',str(node)],cwd=ROOT,text=True,capture_output=True,timeout=110)
 print(p.stdout,end='',flush=True)
 if p.stderr:print(p.stderr,flush=True)
 if p.returncode:raise RuntimeError(f'Node{node} {action} failed; advancement stopped, use stop to request OFF')
 return p.stdout
if args.action=='plan':
 for i,(label,_,expected) in enumerate(STEPS):print(i,label,' | '.join(expected))
 raise SystemExit
if args.action=='status':print(json.dumps(state,indent=2));raise SystemExit
def run_step(index):
 label,destination,expected=STEPS[index]
 print(f'STEP {index}: {label}',flush=True)
 state.update(in_progress=index,failed=True)
 statefile.parent.mkdir(exist_ok=True);statefile.write_text(json.dumps(state,indent=2)+'\n')
 try:
  if destination is None:
   radio('auction-clear',1)
  else:
   radio('destination',destination)
  for node,value in enumerate(expected,1):
   reply=radio('poll',node)
   if f';RESULT=OK;STATE={value};' not in reply:raise RuntimeError(f'Node{node} expected {value}; advancement stopped')
  state.update({'step':index,'label':label,'expected':expected,'failed':False,'optical_confirmation':'user observation required'})
  state.pop('in_progress',None)
  statefile.write_text(json.dumps(state,indent=2)+'\n')
  print('VERIFIED:', ' | '.join(f'Node{n}: {v}' for n,v in enumerate(expected,1)),flush=True)

 except Exception:
  print('FAILED: no further automatic steps will run; indications may be partially applied. Use stop to request OFF.',flush=True)
  raise

if args.action=='auto':
 if state.get('failed'):
  raise SystemExit('Prior step failed: resolve state or use stop before starting auto')
 try:
  run_step(0)
  for pass_number in range(1,args.passes+1):
   print(f'PASS {pass_number}/{args.passes}',flush=True)
   for index in range(1,len(STEPS)):
    time.sleep(args.interval)
    run_step(index)
 except KeyboardInterrupt:
  print('Interrupted. Run stop to request and verify OFF.',flush=True)
  raise SystemExit(130)
else:
 if args.action=='start':index=0
 elif args.action=='stop':index=len(STEPS)-1
 else:
  if state.get('failed'):raise SystemExit('Prior step failed: resolve state or use stop/start before advancing')
  index=state['step']+1
  if index>=len(STEPS):raise SystemExit('Walkthrough complete; use start to repeat')
 run_step(index)
 print('Holding current indications until next or stop.',flush=True)
