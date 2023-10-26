import sys
import os
import json
import subprocess
import re

debug=0

cur_word=sys.argv[1]
comp_cword=int(sys.argv[2])
comp_line=os.getenv('COMP_LINE')


if debug>1:
  print ('\n',file=sys.stderr)
  print (f'1 curword=<{cur_word}>',file=sys.stderr)
  print (f'2 cword=<{comp_cword}>',file=sys.stderr)
  print (f'COMP_LINE=<{comp_line}>',file=sys.stderr)


def printFilter(word,words,option=None) -> None:
    word_lower = word.lower()
    matches=[w for w in words if w.lower().startswith(word_lower)]
    if len(matches) == 1:
        print(re.sub(r'^=','',matches[0]))
    else:
        for w in matches:
            print(re.sub(r'^=','',word + w[len(word):]))
    if option:
        print(option)

def printMsgExit(str,code=0) -> None:
    print(str)
    print(' ')
    print('Options:-o nosort')
    sys.exit(code)

def printSpecHelp(spec, value) -> None:
    atype = spec['type']
    msg_type = atype.replace('std::','')
    semantic = spec.get('semantic')
    allowed = None
    allowed = spec.get('allowed')
    vector = re.search(r'vector<(.*)>',msg_type)
    if vector:
        msg_type='[' + vector.group(1) + ',...]'
    elif atype == 'bool':
        allowed=['True','1','False','0']
        
    if not value:
        msg = f'>Expect <{msg_type}>: {spec["comment"]}'
        if allowed:
            msg += ' (Value one of [' + ','.join(allowed) + '])'
        if semantic and debug>1:
            msg += ' SEMANTIC:{' + ','.join(semantic) + '}'
        printMsgExit(msg)

    if allowed:
        printFilter(value,allowed)
        return
    if msg_type == 'string':
        print(f'File:{value}')
        return

def getApplets() -> dict:
    try:
        result=subprocess.run(['MMVII','GenArgsSpec'],stdout=subprocess.PIPE,stderr=subprocess.DEVNULL,text=True)
    except:
        printMsgExit('>ERROR: MMVII not found.',1)
    try:
        specs=json.loads(result.stdout)
        applets=specs['applets']
    except:
        printMsgExit(">ERROR: Can't get args specification from MMVII.",1)
    return applets

def commandNames(applets) -> None:
    app_names=(a['name'] for a in applets)
    printFilter(cur_word,app_names)

def main() -> int:
    applets=getApplets()
    
    # 1st argument: MMVII command name
    if comp_cword == 1:
        commandNames(applets)
        return 0

    # else get applet specification
    command=comp_line.split()[1].lower()
    applet=[a for a in applets if a['name'].lower() == command]
    if len(applet) != 1:
        sys.exit(0)
    applet=applet[0]

    # Nth first arguments are mandatory
    if comp_cword < len(applet['mandatory'])+2:
        spec = applet['mandatory'][comp_cword-2]
        printSpecHelp(spec, cur_word)
        return 0

    # Others are optionals
    arg_split=re.search(r'([-+a-zA-Z0-9_.]+)(=(.*))?',cur_word)
    #  no '='
    if arg_split==None or arg_split.start(2) < 0:
        normal=sorted((a['name']+'=' for a in applet['optional'] if a['level'] == 'normal'),key=str.lower)
        tuning=sorted((a['name']+'=' for a in applet['optional'] if a['level'] == 'tuning'),key=str.lower)
        common=sorted((a['name']+'=' for a in applet['optional'] if a['level'] == 'global'),key=str.lower)
        printFilter(cur_word,normal+tuning+common,'Options:-o nospace -o nosort')
        return 0
    # got '='
    arg=arg_split.group(1)
    specs = [ s for s in applet.get('optional') if s['name'].lower() == arg.lower() ] 
    if len(specs) != 1:
        return 0
    printSpecHelp(specs[0], arg_split.group(3))
    return 0


if __name__ == '__main__':
    sys.exit(main())
