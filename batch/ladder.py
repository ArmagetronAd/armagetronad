#!/usr/bin/python
# purpose: process ladderlog.txt and generate a usefull ladder league from it

from __future__ import print_function
import math


# collects statistics how two players scored against each other
class PairScore:
    def __init__(self):
        self.weight = 0                      # total statistical weight
        self.weightAverage = self.weight *.5 # weight times the average score of the first player (between zero and one)

    def AddRound( self, scoreOne, scoreTwo ):
        if scoreOne + scoreTwo <= 0:
            return
        self.AddRound( scoreOne/float(scoreOne + scoreTwo ) )

    def AddRound( self, score ):
        self.weight        += 1.0
        self.weightAverage += score

    def Average( self ):
        return self.weightAverage/self.weight

    def Weight( self ):
        return self.weight

    # TODO: persistence

def AddToMatrix( matrix, row, column, value ):
    try:
        matrix[column][row] += value
    except KeyError:
        matrix[column][row] = value

def NormalizeVector( vector ):
    sum = 0.0
    for i in vector:
        sum += vector[i]
    for i in vector:
        vector[i]/=sum

# has one PairScore for each pair of players
class PairScoreDictionary:
    def __init__(self):
        self.pairs = {}

    def GetPairScore( self, playerOne, playerTwo ):
        # sort alphabetically
        if playerOne > playerTwo:
            raise KeyError

        pair = playerOne + " " + playerTwo
        try:
            return self.pairs[pair]
        except KeyError:
            self.pairs[pair] = PairScore()
            return self.pairs[pair] 
    
    # processes scores. scores is supposed to be a list of pairs, each pair consisting of a score and player name. Scores are expected to be non-negative.
    def ReadScore_( self, scores ):
        #print(scores)
        # determine total score sum
        sum = 0
        max = 0
        for pair in scores:
            sum += pair[0]
            if pair[0] > max:
                max = pair[0]
    
        if max <= 0:
            return
    
        # create database
        for first in scores:
            for second in scores:
                if first[1] < second[1]:
                    self.GetPairScore( first[1], second[1] ).AddRound( (max + first[0] - second[0])/(2.0*max) )

    # processes scores. scores is supposed to be a list of pairs, each pair consisting of a score and player name. Sanity checks are made.
    def ReadScore( self, scores ):
        # sum up the scores and find minimum
        minscore = 0
        sum = 0
        for pair in scores:
            sum += pair[0]
            if pair[0] < minscore: minscore = pair[0]

        sum += minscore * len(scores)

        # normalize scores
        normscores = []
        for pair in scores:
            normscores = normscores + [(pair[0] - minscore, pair[1])]
    
        if sum == 0:
            return
   
        self.ReadScore_( normscores )

    # read a log file
    def ReadLog( self, log ):
        scores = []
        for line in log:
            # print(line)
            tokens = line.split()
            if tokens[0] == "ROUND_SCORE":
                scores = scores + [(int (tokens[1]),tokens[2])]
            if tokens[0] == "NEW_ROUND":
                self.ReadScore( scores )
                scores = []

    # generates the ladder out of the collected data
    def GenerateLadder( self ):
        # extract players from pair database, assign each one a number
        players = {}
        for pair in self.pairs:
            for player in pair.split():
                try:
                    x = players[player]
                except KeyError:
                    players[player] = len(players)

        #print(players)

        # generate sparse player-player scoring matrix using a dictionary of dictionaries in column-row order (so normalizing it gets easier)
        matrix = {}
        for i in range(0, len(players)):
            matrix[i] = {}
        
        for pair in self.pairs:
            playerPair = pair.split();
            averager = self.pairs[pair];
            weight = averager.Weight()
            if weight > 0.1:
                if weight > 10:
                    weight = 10
                i = players[playerPair[0]]
                j = players[playerPair[1]]
                scoreFori = averager.Average()
                AddToMatrix(matrix,j,j,-scoreFori*weight);
                AddToMatrix(matrix,i,j,scoreFori*weight);
                AddToMatrix(matrix,j,i,(1-scoreFori)*weight);
                AddToMatrix(matrix,i,i,(scoreFori-1)*weight);

        minDiagonal = 0
        averageDiagonal = 0
        for i in range(0, len(players)):
            diagonal = matrix[i][i]
            if diagonal < minDiagonal: minDiagonal = diagonal
            averageDiagonal += diagonal/len(players)

        print(averageDiagonal)

        maxDeviation = 1
        if minDiagonal < 0:
            maxDeviation = -1/minDiagonal
        print(maxDeviation)

        # initialize score vector
        score = {}
        for i in range(0, len(players)):
            score[i] = 1.0/len(players)

        # load old ladder for better init values
        try:
            for line in open( "newladder.txt" ):
                thisScore, player = line.split()
                thisScore = math.exp(float(thisScore)*math.log(2)/10.0)/len(players)
                try:
                    score[players[player]] = thisScore;
                except KeyError: pass
        except IOError: pass

        #print(matrix)

        # iterate matrix multiplication
        error = 1000
        iter = 0
        while error > .001:

            # matrix multiplication to determine direction to move to
            deltascore = {}
            for i in range(0, len(players)):
                deltascore[i] = -( 1.0/len(players) - score[i] )*averageDiagonal*.1
            for i in matrix:
                column = matrix[i]
                for j in column:
                    deltascore[j] += column[j] * score[i]

            #print(score)
            #print(deltascore)

            # determine minimum and maximum deviation
            error = 0
            for i in range(0, len(players)):
                error += abs(deltascore[i])

            if iter == 100:
                print(error)
                iter = 0   
            iter += 1

            # add just as much of the delta to the score so that no score gets negative
            for i in range(0, len(players)):
               score[i] += deltascore[i] * maxDeviation

            NormalizeVector( score )    
        
        #print(score)
        #print(players)

        ladder = []
        for player in players:
            i = players[player]
            if score[i] > 0:
                ladder = ladder + [( math.log(score[i]*len(players))*10/math.log(2), player )]

        ladder.sort()
        ladder.reverse()

        # print(ladder)
        out = open( "newladder.txt", "w" )
        for entry in ladder:
            out.write(str(entry[0]))
            out.write("\t")
            out.write(entry[1])
            out.write("\n")
        return ladder

scoreDataBase = PairScoreDictionary()
scoreDataBase.ReadLog( open( "ladderlog.txt" ) )
#print("read")
ladder = scoreDataBase.GenerateLadder()

for entry in ladder:
    print(entry[1], entry[0])

