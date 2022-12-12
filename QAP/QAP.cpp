/********** qap.cpp **********/

#include <iostream>
#include <fstream>
#include <vector>
#include "localsolver.h"

using namespace localsolver;
using namespace std;

class Qap {
public:
    /* Number of points */
    int n;

    // Distance between locations
    vector<vector<int> > A;
    // Flow between facilites
    vector<vector<lsint> > B;

    /* Solver. */
    LocalSolver localsolver;

    /* LS Program variables */
    LSExpression p;

    /* Objective */
    LSExpression obj;

    /* Reads instance data */
    void readInstance(const string& fileName) {
        ifstream infile(fileName.c_str());
        if (!infile.is_open()) {
            cerr << "File " << fileName << " cannot be opened." << endl;
            exit(1);
        }
        infile >> n;

        A.resize(n);
        for (int i = 0; i < n; i++) {
            A[i].resize(n);
            for (int j = 0; j < n; j++) {
                infile >> A[i][j];
            }
        }

        B.resize(n);
        for (int i = 0; i < n; i++) {
            B[i].resize(n);
            for (int j = 0; j < n; j++) {
                infile >> B[i][j];
            }
        }

        infile.close();
    }

    void solve(int limit) {
        try {
            /* Declares the optimization model. */
            LSModel model = localsolver.getModel();

            // Permutation such that p[i] is the facility on the location i
            p = model.listVar(n);

            // The list must be complete
            model.constraint(model.count(p) == n);

            // Create B as an array to be accessed by an at operator
            LSExpression arrayB = model.array();
            for (int i = 0; i < n; i++) {
                arrayB.addOperand(model.array(B[i].begin(), B[i].end()));
            }

            // Minimize the sum of product distance*flow
            obj = model.sum();
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    obj += A[i][j] * model.at(arrayB, p[i], p[j]);
                }
            }
            model.minimize(obj);

            model.close();

            /* Parameterizes the solver. */
            LSPhase phase = localsolver.createPhase();
            phase.setTimeLimit(limit);

            localsolver.solve();

        }
        catch (const LSException& e) {
            cout << "LSException:" << e.getMessage() << endl;
            exit(1);
        }
    }

    /* Writes the solution in a file with the following format:
     *  - n objValue
     *  - permutation p */
    void writeSolution(const string& fileName) {
        ofstream outfile(fileName.c_str());
        if (!outfile.is_open()) {
            cerr << "File " << fileName << " cannot be opened." << endl;
            exit(1);
        }
        outfile << n << " " << obj.getValue() << "\n";
        LSCollection pCollection = p.getCollectionValue();
        for (int i = 0; i < n; i++) {
            outfile << pCollection.get(i) << " ";
        }
        outfile << endl;
        outfile.close();
    }
};

int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "Usage: qap inputFile [outputFile] [timeLimit]" << endl;
        exit(1);
    }

    const char* instanceFile = argv[1];
    const char* solFile = argc > 2 ? argv[2] : NULL;
    const char* strTimeLimit = argc > 3 ? argv[3] : "300";

    Qap model;
    model.readInstance(instanceFile);
    model.solve(atoi(strTimeLimit));
    if (solFile != NULL)
        model.writeSolution(solFile);
    return 0;
}