import json
import time
import sys
import random

if len(sys.argv) != 2:
	print "WRONG PARAMETERS: python whisper.py topofile.json"


print "Parameter  is"+str(sys.argv[1])

data = json.load(open(sys.argv[1]))

MINRANKHOPINCREASE=256
THRESHOLD=512
HIGHRANK=10*MINRANKHOPINCREASE

#create nodes
nodes={}

for n in data['motes']:

    nodes[n['id']]={
	'id':  n['id'],
	'rank':  256,
	'parent': -1,
	'neighbors': {},
	'children': [],
    }


#fill relationships
for n in data['motes']:

	    for e in data['connections']:
		if e['pdr'] >= 0.5:	#only consider links with high pdr
			if n['id'] == e['fromMote']:
			    nodes[n['id']]['neighbors'][e['toMote']]={}
			    nodes[n['id']]['neighbors'][e['toMote']]['etx']=(1/e['pdr'])

			    nodes[e['toMote']]['neighbors'][n['id']]={}
			    nodes[e['toMote']]['neighbors'][n['id']]['etx']=(1/e['pdr'])

#initialize ranks
def initRanks():

	for n in nodes.values():
		for (neigh, val) in n['neighbors'].items():

		    nodes[n['id']]['neighbors'][neigh]['tentativeRank']=nodes[neigh]['rank']+nodes[n['id']]['neighbors'][neigh]['etx']*MINRANKHOPINCREASE
	            #check if the network is stable
		    assert n['rank'] <= (THRESHOLD + nodes[n['id']]['neighbors'][neigh]['tentativeRank'])

#initialize parentship
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

#fill the paths from any node to the root
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

#get the parent candidate with the minimum rank
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

def getRandomTestingNodes(P):
	a=0	#t
	b=0	#ct
	c=0	#dt
	dtNotFound=True
	it=0

	
	while dtNotFound:
		a=int(random.uniform(2, len(nodes))	#pick randomly a target node
		b=0
		c=0

		if nodes[a]['parent']!=0:
			#not the root node
			
			if len(nodes[a]['neighbors'])>1:			
				b=nodes[a]['parent']	#select its current parent
				neighs= [n for n in nodes[a]['neighbors'] if (n!=nodes[a]['parent'] and nodes[n]['parent']!=a)]
				
				if len(neighs)>0:
					c= neighs[int(random.randint(0, len(neighs)-1))]	#choose a new parent randomly from its neighbors
				if a!=0 and b!=0 and c!=0:
					if c!=1:
						if a not in P[c]:	#avoiding loops, not choose a children
							dtNotFound=False	
					else:# when ct is the root, can't be loops
						dtNotFound=False
		it+=1
		if it>500:	#should not happen, only when a valid use case can't be find in the network
			print "Impossible to find a proper use case"
			assert False
	return (a,b,c)


failure=0
times=[]
totalCommands=[]
totalType1=[]
totalType2=[]
w=0

initRanksAndParents()

#here the experiment start
while w < 10000:

	#for measuring the executing time
	start_time = time.time()

	#it initialized the ranks in the neighbors
	initRanks()

	for n in nodes.items():
		lis2=[]
		lis=n[1]['neighbors'].keys()
		for l in lis:
			lis2.append(l)

	#paths of every node
	P=fillPs()
	
	#heads of the branches
	heads=set([P[b][0] for b in P.keys()])

	I={}
	for h in heads:
		I[h]=0
	neighboursProcessed=0
	needExtraDio=[]
	needWhisperNode=False

	(t,ct,dt)=getRandomTestingNodes(P)

	#constraint nodes
	CN=[]
	#cadidates as parent
	CP={}

	target=[val for val in nodes.values() if val['id'] == t][0] 

	new=[val for val in nodes.values() if val['id'] == dt][0] 

	old=[val for val in nodes.values() if val['id'] == ct][0] 

	CP[target['id']]=getCPs(target)

	tentativeRankWithDt= target['neighbors'][dt]['tentativeRank']
	tentativeRankWithCt= target['neighbors'][ct]['tentativeRank']
	minRankOfCPdt=getCandidatesWithMinRank(getCPs(nodes[t]))

	#number of primitives needed in each case
	numType1=0
	numType2=0
	numType3=0
	numType4=0

	#other than the desired parent
	potentialParents=[n for n in CP[target['id']] if n[1]['tentativeRank'] <= tentativeRankWithDt and n[0] != dt]
	print potentialParents
	for pp in potentialParents:
		if pp[0] == 1:
			potentialParents.remove(pp)
			numType1+=1
			continue
		if t in P[pp[0]]:	#we can remove this potential parent because is a child of t. if we increase te rank in t, his rank will also eventually be increased
			potentialParents.remove(pp)

	#only if there is one or more potential parents (besides the desired parent)
	for n in potentialParents:
			if n[0]==1:
				continue
	
			neighboursProcessed+=1

			#same branch as dt
			if P[n[0]][0]==P[dt][0]:

				if n[1]['tentativeRank'] == tentativeRankWithDt:
						needWhisperNode=True	#a whisper node is needed, abort
						break
				else:
					assert n[1]['tentativeRank'] < tentativeRankWithDt
					if P[n[0]][0]==P[t][0]:
						needWhisperNode=True	#a whisper node is needed, abort
						break
					else:	
						if len([ i for i in potentialParents if i[0]!=n[0] and i[0]!=1 and P[i[0]][0]==P[n[0]][0] ])>0:
							needWhisperNode=True	#a whisper node is needed, abort
							break
						else:
							#if comes here is because it has the same rank as dt, it is enough to increase it a bit	
							rankdiff = abs( tentativeRankWithDt - n[1]['tentativeRank']) + 1
							impossibleIncreaseRankInBranchConfirmed=False
							stack=[]
							stack.append(ct)
							
							#if it is the root, it wont have any CN
							if ct==1:
								vertex = stack.pop()
							i=0
							while len(stack)!=0:
								i=i+1
								vertex = stack.pop()
								if P[vertex][0] == P[dt][0]:
									#Impossible to increase the rank. a whisper node is needed, abort
									impossibleIncreaseRankInBranchConfirmed=True
									for i in heads:	#reset Is
										I[P[i][0]]=0
									needWhisperNode=True	
									break
								
								CNsOfTheBranchOfThisNode=[]
								#get all nodes in the branch of P[vertex][0]
								for nodeInBranch,branch in P.items():
									if branch[0]==P[vertex][0]:
										(cnodes,r) = getCandidatesWithMinRank(getCPs(nodes[nodeInBranch]))
										for (c) in cnodes:
											if abs(nodes[nodeInBranch]['rank'] - r + rankdiff) >= THRESHOLD:
												if c not in P[n[0]]:
													if nodeInBranch!=t:
														CNsOfTheBranchOfThisNode.append(nodeInBranch)
								currentDiff=0
								#constraint node is in the branch of the node
								for cn in CNsOfTheBranchOfThisNode:
									(cnodes,r) = getCandidatesWithMinRank(getCPs(nodes[cn]))
									for (c) in cnodes:
										if c!=1:
											if c not in P[vertex]:
												if I[P[c][0]]==0:
													if c not in stack:
														stack.append(c)
														currentDiff=abs(nodes[cn]['rank']-nodes[c]['rank']-MINRANKHOPINCREASE+rankdiff-THRESHOLD)

								if I[P[vertex][0]] <= rankdiff:
									I[P[vertex][0]]=rankdiff
								rankdiff=currentDiff

							if not impossibleIncreaseRankInBranchConfirmed:
								needExtraDio.append(n[0])

			else:
				#candidate is not in the same branch as dt

				assert n[1]['tentativeRank'] <= tentativeRankWithDt

				#if comes here is because it has the same rank as dt, it is enough to increase it a bit					
				impossibleIncreaseRankInBranchConfirmed=False
				stack=[]
				stack.append(n[0])
				rankdiff = abs( tentativeRankWithDt - n[1]['tentativeRank']) + 1
				i=0
				while len(stack)!=0:
					i=i+1
					vertex = stack.pop()
					if P[vertex][0] == P[dt][0]:
						##a whisper node is needed, abort
						impossibleIncreaseRankInBranchConfirmed=True
						needWhisperNode=True
						for i in heads:		#reset I values
							I[P[i][0]]=0
						break
					
					CNsOfTheBranchOfThisNode=[]
					#get all nodes in the branch of P[vertex][0]
					
					for nodeInBranch,branch in P.items():
						if branch[0]==P[vertex][0]:
							(cnodes,r) = getCandidatesWithMinRank(getCPs(nodes[nodeInBranch]))
							for (c) in cnodes:
								if abs(nodes[nodeInBranch]['rank'] - r + rankdiff) >= THRESHOLD:
									if c not in P[n[0]]:
										if nodeInBranch!=t:
											CNsOfTheBranchOfThisNode.append(nodeInBranch)
					currentDiff=0
					for cn in CNsOfTheBranchOfThisNode:
						(cnodes,r) = getCandidatesWithMinRank(getCPs(nodes[cn]))
						for (c) in cnodes:
							if c!=1:
								if c not in P[vertex]:
									if I[P[c][0]]==0:
										if c not in stack:
											stack.append(c)
											#rank to be added in the next branch
											currentDiff=abs(nodes[cn]['rank']-nodes[c]['rank']-MINRANKHOPINCREASE+rankdiff-THRESHOLD)
											
					if I[P[vertex][0]] <= 0:
						I[P[vertex][0]]=rankdiff
					rankdiff=currentDiff
			#stop the test
			if needWhisperNode:
				break	

	#these statemets prepare the primitives
	if not needWhisperNode:
		fail=False
		print "RESULTS!----"
		if neighboursProcessed==0:
			if numType1>0:
				print "Primitive: Send REMOTE DIO directly from the ROOT to target t="+str(t)+" with rank "+str(HIGHRANK)+" using a local link"
			numType1+=1
			print "Primitive: Send REMOTE DIO from the ROOT to target t="+str(t)+" with rank "+str(HIGHRANK)
		else:
			if len(needExtraDio)>0:
				numType1+=1
				print "Primitive: Send REMOTE DIO from the ROOT to target t="+str(t)+" with rank "+str(HIGHRANK)
		
				for i in heads:
					if I[i]!=0:
						numType2+=1
						print "Primitive: Send PROPAGATING DIO from the ROOT towards branch t="+str(i)+" with rank "+str(I[i])

				for extraDios in needExtraDio:
					numType1+=1
					print "Primitive: Send REMOTE EXTRA DIO from the ROOT to target t="+str(t)+" with rank "+str(HIGHRANK)+" this time will pas through "+str(extraDios)
			else:
				numType1+=1
				print "Primitive: Send REMOTE DIO from the ROOT to target t="+str(t)+" with rank "+str(HIGHRANK)
				for i in heads:
					if I[i]!=0:
						numType2+=1
						print "Primitive: Send PROPAGATING DIO from the ROOT towards branch t="+str(i)+" with rank "+str(I[i])
	else:
		print "WHISPER NODES ARE NEEDED"
		failure+=1
		fail=True
	t=(time.time() - start_time)
	times.append(t)
	totalCommands.append(numType1+numType2)
	totalType1.append(numType1)
	totalType2.append(numType2)
	if not fail:
		print "Test "+str(w)+": t="+str(t)+" ct="+str(ct)+" dt="+str(dt)+" Messages = "+str(numType1+numType2)+" : numType1 "+str(numType1)+" : numType2 "+str(numType2)+" OK "+" secs "+str(t)
	else:
		print "WHISPER NODES ARE NEEDED FOR TARGET "+str(t)
	w+=1
	#end test

#end all the tests

mpe=0
type1rate=0
type2rate=0
print "END EXPERIMENT--------------------------------------------------------------"

print "Stats: Success "+str(w-failure)+" failure "+str(failure)+" / "+str(w)
if w!=failure:
	mpe=float(sum(totalCommands))/(w-failure)
if sum(totalCommands)!=0:
	type1rate=100*float(sum(totalType1))/sum(totalCommands)
	type2rate=100*float(sum(totalType2))/sum(totalCommands)

print "Stats: Success "+str(w-failure)+" failure "+str(failure)+" / "+str(w)+" messages per exp "+str(mpe)+" total commands "+str()+" numType1 "+str(type1rate)+" : numType2 "+str(type2rate)+" average time "+str(reduce(lambda x, y: x + y, times) / float(len(times)))+" file "+str(sys.argv[1])

out="Stats: Success "+str(w-failure)+" failure "+str(failure)+" / "+str(w)+" messages per exp "+str(mpe)+" total commands "+str()+" numType1 "+str(type1rate)+" : numType2 "+str(type2rate)+" average time "+str(reduce(lambda x, y: x + y, times) / float(len(times)))+" file "+str(sys.argv[1])+"\n"

with open("outlogs.ods",'a') as f:
    f.write(out)



