import networkx as nx

import matplotlib.pyplot as plt
import json
import time
import sys
import random
#plotly

MINRANKHOPINCREASE=256
THRESHOLD=512
HIGHRANK=10*MINRANKHOPINCREASE


if len(sys.argv) != 2:
	print "WRONG PARAMETERS: plotgraph.py topofile.json"


print "Parameter "+str(sys.argv[1])

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


data = json.load(open(sys.argv[1]))
G=nx.DiGraph()

nodes={}

for n in data['motes']:

    nodes[n['id']]={
	'id':  n['id'],
	'rank':  256,
	'parent': -1,
	'neighbors': {},
	'children': [],
    }
    G.add_node(n['id'])
    

#fill relationship
for n in data['motes']:
	    for e in data['connections']:
		if e['pdr'] > 0.5:
			if n['id'] == e['fromMote']:
			    nodes[n['id']]['neighbors'][e['toMote']]={}
			    nodes[n['id']]['neighbors'][e['toMote']]['etx']=(1/e['pdr'])

			    nodes[e['toMote']]['neighbors'][n['id']]={}
			    nodes[e['toMote']]['neighbors'][n['id']]['etx']=(1/e['pdr'])


print "Init"
initRanksAndParents()	

for n in nodes.items():
	if n[1]['parent']!=-1:
		G.add_edge(n[0],n[1]['parent'],color='r')
	for neigh in n[1]['neighbors']:
		
		if n[1]['parent']!=neigh and nodes[neigh]['parent']!=n[0]:
			G.add_edge(n[0],neigh,color='k')

edges = G.edges()		
colors = [G[u][v]['color'] for u,v in edges]
nx.draw(G,with_labels=True,edges=edges,edge_color=colors)

plt.show()
