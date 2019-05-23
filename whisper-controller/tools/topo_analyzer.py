import json
import time
import sys
import random

#get the avg number of neighbors per node and the avg hops to the root

if len(sys.argv) != 2:
	print "WRONG PARAMETERS: topo_analyzer.py topofile.json"


print "Parameter "+str(sys.argv[1])

data = json.load(open(sys.argv[1]))

MINRANKHOPINCREASE=256
THRESHOLD=512
HIGHRANK=10*MINRANKHOPINCREASE

#create nodes
nodes={}

for n in data['motes']:

    #print n
    nodes[n['id']]={
	'id':  n['id'],
	'rank':  256,
	'parent': -1,
	'neighbors': {},
	'children': [],
    }

#fill relationship
for n in data['motes']:

	    for e in data['connections']:
		if e['pdr'] > 0.5:
			if n['id'] == e['fromMote']:
			    nodes[n['id']]['neighbors'][e['toMote']]={}
			    nodes[n['id']]['neighbors'][e['toMote']]['etx']=(1/e['pdr'])

			    nodes[e['toMote']]['neighbors'][n['id']]={}
			    nodes[e['toMote']]['neighbors'][n['id']]['etx']=(1/e['pdr'])

def initRanks():

	for n in nodes.values():
			for (neigh, val) in n['neighbors'].items():
		
		    		nodes[n['id']]['neighbors'][neigh]['tentativeRank']=nodes[neigh]['rank']+nodes[n['id']]['neighbors'][neigh]['etx']*MINRANKHOPINCREASE
				#for check stability
				assert n['rank'] <= (THRESHOLD + nodes[n['id']]['neighbors'][neigh]['tentativeRank'])

def initRanksAndParents():

	stack=[]
	stack.append(nodes[1])
	i=0

	visited=[]
	visited.append(nodes[1]['id'])
	while len(stack) !=0:
		
		ver=stack.pop()
		for neigh in ver['neighbors']:
			if neigh in visited:
				continue
			for e in data['connections']:
				if( (ver['id'] == e['fromMote'] and neigh == e['toMote']) or ((ver['id'] == e['toMote'] and neigh == e['fromMote'])) ) and e['rpl']==1:
					stack.append(nodes[neigh])
					nodes[neigh]['parent']=ver['id']
					nodes[neigh]['rank']=(1/e['pdr'])*MINRANKHOPINCREASE+nodes[ver['id']]['rank']
					nodes[nodes[neigh]['parent']]['children'].append(neigh)	
					visited.append(nodes[neigh]['id'])

#fill the paths from a node to the root
def fillPs():
	
	P={}
	for n in nodes.values():
	
		if n['id']==1:
			continue
		
		i = n
		l=[]
		
		l.append(i['id'])
		

		while i['parent'] != 1:
			if int(i['parent'])==1:
				break
			p=int(i['parent'])
			i=[val for val in nodes.values() if val['id'] == p][0]
		
			l.append(i['id'])

		P[n['id']]=list(reversed(l))
	return P


#get candidate parents
def getCPs(node):
	l=[]
	for neigh in node['neighbors'].items():
		if neigh[0] != node['parent']:
			l.append(neigh)
	return l


def getCandidatesWithMinRank(candidates):
	minval=999999
	minCands=[]
	
	for c,val in candidates:
		#print (c, val)
		if val['tentativeRank'] < minval:
			minCands=[]
			minval=val['tentativeRank']
			minCands.append(c)
		else:
			if val['tentativeRank'] == minval:
				minCands.append(c)
	return (minCands,minval)


initRanksAndParents()
initRanks()

P=fillPs()
print "Paths "+str(P)

hops=0.0
for node in P:
	hops=hops+len(P[node])

avehops=hops/len(P)

neighs=0.0
for n in nodes:
	neighs=neighs+len(nodes[n]['neighbors'])

aveneighs=neighs/len(nodes)

print "avehops "+str(avehops) +" aveneighs "+str(aveneighs)+" file "+str(sys.argv[1])
out="avehops "+str(avehops) +" aveneighs "+str(aveneighs)+" file "+str(sys.argv[1])+"\n"
with open("topo.ods",'a') as f:
    f.write(out)



