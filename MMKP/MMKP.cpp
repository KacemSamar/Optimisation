
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <string.h>
#include <ctime>
using namespace std;

int generateRandomNumber(int min, int max)
{
    int r;
    double randd = ((double)rand() / ((double)(RAND_MAX)+(double)(1)));
    r = (int)(randd * (double)(max - min + 1)) + min;
    return r;
}

double generateDoubleRandomNumber()
{
    return ((double)rand() / ((double)(RAND_MAX)+(double)(1)));
}

int randomCrossover()
{
    int cc = (int)((double)generateDoubleRandomNumber() * (double)2);
    return cc;
}

class GreedyObject
{
public:
    double ratio;
    int group;
    int index;

    GreedyObject()
    {
    }

    GreedyObject(int g, double r, int i)
    {
        ratio = r;
        group = g;
        index = i;
    }

    GreedyObject(const GreedyObject& copy)
    {
        this->ratio = copy.ratio;
        this->group = copy.group;
        this->index = copy.index;
    }

    bool operator < (const GreedyObject& right) const
    {
        return this->ratio > right.ratio;
    }

    bool operator > (const GreedyObject& right) const
    {
        return this->ratio < right.ratio;
    }
};

class Group
{
public:
    int group;
    vector<GreedyObject> objects;

    Group()
    {
    }

    Group(int g)
    {
        group = g;
    }

    int getGroup()
    {
        return group;
    }

    vector<GreedyObject> getObjects()
    {
        return objects;
    }

    void sortObjects()
    {
        sort(objects.begin(), objects.end());
    }

    GreedyObject getObjectAtIndex(int in)
    {
        return objects.at(in);
    }

    void addObject(GreedyObject obj)
    {
        objects.push_back(obj);
    }

    GreedyObject getObject(int index)
    {
        return objects.at(index);
    }
};

int NUMBER_GROUPS = 0; 
int NUMBER_OBJECTS_IN_GROUP = 0; 
int NUMBER_CONSTRAINTS = 0;
int*** CONSTRAINTS; 
int* CAPACITIES; 
int** VALUES;
int VALID_SOLUTION_ITERATIONS;

int ANNEALING_ITERATIONS = 100000;
int RANDOM_CHANGES; 
const double ANNEALING_FACTOR = 10;

vector<GreedyObject> GREEDY_OBJECTS;
vector<Group> GROUPS;
GreedyObject** RATIO_OBJECTS;



void processData(char* filename)
{
    FILE* file;
    file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("File Not Found in Current Directory.");
        exit(1);
    }

    char* line = new char[1000];
    fgets(line, 1000, file);
    while (strlen(line) <= 1)
    {
        fgets(line, 1000, file);
    }
    char* tok = strtok(line, " ");
    NUMBER_GROUPS = atoi(tok);
    tok = strtok(NULL, " ");
    NUMBER_OBJECTS_IN_GROUP = atoi(tok);
    tok = strtok(NULL, " ");
    NUMBER_CONSTRAINTS = atoi(tok);

    int i = 0;
    fgets(line, 1000, file);

    CAPACITIES = new int[NUMBER_CONSTRAINTS];
    tok = strtok(line, " ");
    for (i = 0; i < NUMBER_CONSTRAINTS; i++)
    {
        CAPACITIES[i] = atoi(tok);
        tok = strtok(NULL, " ");
    }

    VALUES = new int* [NUMBER_GROUPS];
    CONSTRAINTS = new int** [NUMBER_GROUPS];
    fgets(line, 1000, file);
    for (i = 0; i < NUMBER_GROUPS; i++)
    {
        CONSTRAINTS[i] = new int* [NUMBER_OBJECTS_IN_GROUP];
        VALUES[i] = new int[NUMBER_OBJECTS_IN_GROUP];
        fgets(line, 1000, file);
        tok = strtok(line, " ");
        for (int g = 0; g < NUMBER_OBJECTS_IN_GROUP; g++)
        {
            CONSTRAINTS[i][g] = new int[NUMBER_CONSTRAINTS];
            int value = atoi(tok);
            VALUES[i][g] = value;

            for (int c = 0; c < NUMBER_CONSTRAINTS; c++)
            {
                tok = strtok(NULL, " ");
                CONSTRAINTS[i][g][c] = atoi(tok);
            }
            fgets(line, 1000, file);
            tok = strtok(line, " ");
        }
    }

    delete[] line;
}

class MNode
{
public:
    int* objects;
    int* weights;
    int value;

    MNode()
    {
        objects = NULL;
    }

    MNode(int* o)
    {
        objects = new int[NUMBER_GROUPS];
        for (int i = 0; i < NUMBER_GROUPS; i++)
        {
            objects[i] = o[i];
        }
        calculateValue();
        calculateWeights();
    }

    ~MNode()
    {
        if (objects != NULL)
            delete[] objects;

        if (weights != NULL)
            delete[] weights;
    }

    MNode(const MNode& copy)
    {
        int* o = copy.objects;
        objects = new int[NUMBER_GROUPS];
        for (int i = 0; i < NUMBER_GROUPS; i++)
        {
            objects[i] = o[i];
        }
        calculateValue();
        calculateWeights();
    }

    MNode& operator = (const MNode& copy)
    {
        if (objects != NULL)
            delete[] objects;

        if (weights != NULL)
            delete[] weights;

        int* o = copy.objects;
        objects = new int[NUMBER_GROUPS];
        for (int i = 0; i < NUMBER_GROUPS; i++)
        {
            objects[i] = o[i];
        }
        calculateValue();
        calculateWeights();
        return *this;
    }

    bool operator < (const MNode& right)
    {
        return this->value > right.value;
    }

    bool operator > (const MNode& right)
    {
        return this->value < right.value;
    }

    void calculateValue()
    {
        value = 0;
        for (int i = 0; i < NUMBER_GROUPS; i++)
        {
            value += VALUES[i][objects[i]];
        }
    }

    void calculateWeights()
    {
        weights = new int[NUMBER_CONSTRAINTS];
        for (int i = 0; i < NUMBER_CONSTRAINTS; i++)
        {
            weights[i] = 0;
            for (int j = 0; j < NUMBER_GROUPS; j++)
            {
                int aas = objects[j];
                weights[i] += CONSTRAINTS[j][aas][i];
            }
        }
    }

    bool violatesConstraints() const
    {
        for (int i = 0; i < NUMBER_CONSTRAINTS; i++)
        {
            if (weights[i] > CAPACITIES[i])
            {
                return true;
            }
        }
        return false;
    }

    int constraintViolation() const
    {
        int violation = 0;
        for (int i = 0; i < NUMBER_CONSTRAINTS; i++)
        {
            if (weights[i] > CAPACITIES[i])
            {
                violation += (weights[i] - CAPACITIES[i]);
            }
        }
        return violation;
    }

    void changeSelection(int group, int assignment)
    {
        if (objects[group] == assignment)
            return;

        int previous = objects[group];
        for (int i = 0; i < NUMBER_CONSTRAINTS; i++)
        {
            weights[i] -= CONSTRAINTS[group][previous][i];
        }

        value -= VALUES[group][previous];

        for (int i = 0; i < NUMBER_CONSTRAINTS; i++)
        {
            weights[i] += CONSTRAINTS[group][assignment][i];
        }

        value += VALUES[group][assignment];

        objects[group] = assignment;
    }

    int fitness()
    {
        if (violatesConstraints())
        {
            return -100;
        }
        else
        {
            return value;
        }
    }

    int getValueOfIndex(int index)
    {
        return objects[index];
    }

    MNode clone()
    {
        int* obj = new int[NUMBER_GROUPS];

        for (int i = 0; i < NUMBER_GROUPS; i++)
        {
            obj[i] = objects[i];
        }

        MNode clone(obj);
        delete[] obj;
        return clone;
    }
};

inline void simulatedAnnealing(MNode& n)
{
    MNode node = n.clone();
    MNode best = n.clone();
    for (int i = 0; i < ANNEALING_ITERATIONS; i++)
    {
        for (int j = 0; j < RANDOM_CHANGES; j++)
        {
            int rand1 = generateRandomNumber(0, NUMBER_GROUPS - 1);
            int rand2 = generateRandomNumber(0, NUMBER_OBJECTS_IN_GROUP - 1);
            while (node.getValueOfIndex(rand1) == rand2)
            {
                rand1 = generateRandomNumber(0, NUMBER_GROUPS - 1);
                rand2 = generateRandomNumber(0, NUMBER_OBJECTS_IN_GROUP - 1);
            }
            node.changeSelection(rand1, rand2);
        }


        if (node.violatesConstraints())
        {
            vector<GreedyObject> vec;
            for (int k = 0; k < NUMBER_GROUPS; k++)
            {
                int sel = node.getValueOfIndex(k);
                double ratio = RATIO_OBJECTS[k][sel].ratio;
                GreedyObject obj(k, ratio, sel);
                vec.push_back(obj);
            }

            sort(vec.begin(), vec.end());

            int count = vec.size() - 1;
            while (node.violatesConstraints())
            {
                int val = -1;
                if (count < 0)
                {
                    val = generateRandomNumber(0, NUMBER_GROUPS - 1);
                }
                else
                {
                    GreedyObject obj = vec.at(count);
                    count--;
                    val = obj.group;
                }

                Group group = GROUPS.at(val);
                for (int i = 0; i < NUMBER_OBJECTS_IN_GROUP; i++)
                {
                    GreedyObject obj = group.objects.at(i);
                    int index = obj.index;
                    int group = obj.group;
                    int previous = node.getValueOfIndex(group);
                    int pv = node.constraintViolation();
                    node.changeSelection(group, index);
                    int nv = node.constraintViolation();
                    if (pv < nv)
                    {
                        node.changeSelection(group, previous);
                    }
                }
            }
        }

        int numObjects = NUMBER_GROUPS * NUMBER_OBJECTS_IN_GROUP;

        for (int ii = 0; ii < numObjects; ii++)
        {
            GreedyObject obj = GREEDY_OBJECTS.at(ii);
            int index = obj.index;
            int group = obj.group;
            int previous = node.getValueOfIndex(group);
            int preValue = node.fitness();
            node.changeSelection(group, index);
            if (node.violatesConstraints() || preValue > node.fitness())
            {
                node.changeSelection(group, previous);
            }
        }

        printf("\nSolution numero %d = %d ", i+1, best.fitness());

        if (best.fitness() < node.fitness())
        {
            best = node;
        }
        else
        {

            double probability = (double)i / (double)ANNEALING_ITERATIONS;
            probability *= ANNEALING_FACTOR;
            double rand = generateDoubleRandomNumber();
            if (rand < probability)
                node = best;
        }
    }

    n = best;
}

MNode greedyAlgorithm()
{
    RATIO_OBJECTS = new GreedyObject * [NUMBER_GROUPS];
    for (int i = 0; i < NUMBER_GROUPS; i++)
    {
        RATIO_OBJECTS[i] = new GreedyObject[NUMBER_OBJECTS_IN_GROUP];
        Group group(i);
        for (int j = 0; j < NUMBER_OBJECTS_IN_GROUP; j++)
        {
            double weight = 0;
            for (int k = 0; k < NUMBER_CONSTRAINTS; k++)
            {
                weight += CONSTRAINTS[i][j][k];
            }

            double value = VALUES[i][j];
            double ratio = value / weight;
            GreedyObject obj(i, ratio, j);
            GREEDY_OBJECTS.push_back(obj);
            GreedyObject o(i, ratio, j);
            group.addObject(o);
            RATIO_OBJECTS[i][j] = o;

        }
        group.sortObjects();
        GROUPS.push_back(group);
    }

    sort(GREEDY_OBJECTS.begin(), GREEDY_OBJECTS.end());

    int* flags = new int[NUMBER_GROUPS];
    for (int i = 0; i < NUMBER_GROUPS; i++)
    {
        flags[i] = 0;
    }

    int numObjects = NUMBER_GROUPS * NUMBER_OBJECTS_IN_GROUP;
    int* objects = new int[NUMBER_GROUPS];
    int* chain = new int[NUMBER_GROUPS];
    int count = 0;
    for (int i = 0; i < numObjects; i++)
    {
        GreedyObject obj = GREEDY_OBJECTS.at(i);
        int group = obj.group;
        int index = obj.index;
        if (flags[group] == 0)
        {
            chain[count] = group;
            count++;
            objects[group] = index;
            flags[group] = 1;
        }
    }
    MNode node(objects);

    count = NUMBER_GROUPS - 1;
    int nn = 0;
    while (node.violatesConstraints())
    {
        int val = -1;
        if (count < 0)
            val = generateRandomNumber(0, NUMBER_GROUPS - 1);
        else
            val = chain[count];

        count--;
        Group group = GROUPS.at(val);
        for (int i = 0; i < NUMBER_OBJECTS_IN_GROUP; i++)
        {
            GreedyObject obj = group.objects.at(i);
            int index = obj.index;
            int group = obj.group;
            int previous = node.getValueOfIndex(group);
            int pv = node.constraintViolation();
            node.changeSelection(group, index);
            int nv = node.constraintViolation();
            if (pv < nv)
            {
                node.changeSelection(group, previous);
            }
        }

        nn++;
        if (nn > VALID_SOLUTION_ITERATIONS)
        {
            printf("A valid solution might NOT exist for the instance! Quitting.");
            exit(0);
        }
    }


    for (int i = 0; i < numObjects; i++)
    {
        GreedyObject obj = GREEDY_OBJECTS.at(i);
        int index = obj.index;
        int group = obj.group;
        int previous = node.getValueOfIndex(group);
        int preValue = node.fitness();
        node.changeSelection(group, index);
        if (node.violatesConstraints() || preValue > node.fitness())
        {
            node.changeSelection(group, previous);
        }
    }

    

    delete[] chain;
    delete[] objects;
    delete[] flags;
    return node;
}

int main(int argc, char* argv[])
{
    srand(time(NULL));

    if (argc < 3)
    {
        printf("\nUsage: ./a.out filename number_iterations\n\n");
        exit(0);
    }

    printf("Data File name: %s\n", argv[1]);
    printf("Number of iterations: %s\n", argv[2]);


    processData(argv[1]);
    ANNEALING_ITERATIONS = atoi(argv[2]);

    VALID_SOLUTION_ITERATIONS = NUMBER_GROUPS * NUMBER_OBJECTS_IN_GROUP;


    if (NUMBER_GROUPS < 50)
        RANDOM_CHANGES = 3;
    else
        RANDOM_CHANGES = 5;


    MNode node = greedyAlgorithm();

    simulatedAnnealing(node);

    printf("\nFinal Solution: %d\n", node.fitness());

    for (int i = 0; i < NUMBER_GROUPS; i++)
    {
        printf("%d ", node.getValueOfIndex(i));
    }

    delete[] CAPACITIES;
    for (int i = 0; i < NUMBER_GROUPS; i++)
    {
        delete[] VALUES[i];
    }
    delete[] VALUES;
    for (int i = 0; i < NUMBER_GROUPS; i++)
    {
        for (int j = 0; j < NUMBER_OBJECTS_IN_GROUP; j++)
        {
            delete[] CONSTRAINTS[i][j];
        }

        delete[] CONSTRAINTS[i];
    }
    delete[] CONSTRAINTS;
    for (int i = 0; i < NUMBER_GROUPS; i++)
    {
        delete RATIO_OBJECTS[i];
    }

    delete[] RATIO_OBJECTS;

    return 0;
}