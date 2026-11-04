"""Three-node walkthrough driven by the controller destination-only bench input.
Uses controller business flow; the deferred application event interface is not implemented.
"""
import argparse,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
PYTHON=ROOT.parent/'Tools/marketsys-uart-venv/bin/python'
# Only destinations are sent. All resulting states are computed by the controller.
STEPS=[
 ('All OFF',None,['OFF','OFF','OFF']),
 ('New head to Node1',1,['ACTIVE','OFF','OFF']),
 ('Another head to Node1: HOLD_OPEN',1,['HOLD_OPEN','OFF','OFF']),
 ('New head to Node2',2,['LAST','ACTIVE','OFF']),
 ('Another head to Node2: HOLD_OPEN, previous LAST off',2,['OFF','HOLD_OPEN','OFF']),
 ('New head to Node3; older LAST off',3,['OFF','LAST','ACTIVE']),
 ('Another head to Node3: HOLD_OPEN, previous LAST off',3,['OFF','OFF','HOLD_OPEN']),
 ('Return to Node2',2,['OFF','ACTIVE','LAST']),
 ('Return to Node1; older LAST off',1,['ACTIVE','LAST','OFF']),
 ('Finish all OFF',None,['OFF','OFF','OFF']),
]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('action',choices=['plan','start','next','stop','status'])
args=parser.parse_args()
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
if args.action=='start':index=0
elif args.action=='stop':index=len(STEPS)-1
else:
 if state.get('failed'):raise SystemExit('Prior step failed: resolve state or use stop/start before advancing')
 index=state['step']+1
 if index>=len(STEPS):raise SystemExit('Walkthrough complete; use start to repeat')
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
 state={'step':index,'label':label,'expected':expected,'failed':False,'optical_confirmation':'user observation required'}
 statefile.write_text(json.dumps(state,indent=2)+'\n')
 print('VERIFIED:', ' | '.join(f'Node{n}: {v}' for n,v in enumerate(expected,1)),flush=True)
 print('Holding current indications until next or stop; no timer advances this test.',flush=True)
except Exception:
 print('FAILED: no further automatic steps will run; indications may be partially applied. Use stop to request OFF.',flush=True)
 raise
