#include <array>
#include <iostream>
#include <span>
#include <vector>
#include <cmath>
#include <algorithm>
#include <stack>
#include <queue>
// -----------------------------------------------------------------------------

// You are free to add any STL includes above this comment, below the --line--.
// DO NOT add "using namespace std;" or include any other files/libraries.
// Also DO NOT add the include "bits/stdc++.h"

// OPTIONAL: Add your helper functions and data structures here

// best 1 is the one which gives long pattern 
// we want to get best 1 in a row to avoid repetations ,we will use below function
int best_index_in_row(std::vector<std::vector<int>> &matrix, int i) {   
   int best_j = -1;
   int max_count = 1;
   int n = matrix[0].size();
   int m = matrix.size();
   for (int j = 0; j < n; j++) {
       if (matrix[i][j] == 1) {
        
           int count = 1; 
           while (true) {
               if (i + count >= m || j + count >= n) { // index should go out of bound
                   break;
               } else {
                   if (matrix[i + count][j + count] == 1) {
                       count++;
                   } else {
                       break;
                   }
               }
           }

           if (count >= max_count) {
               if (count == max_count) { // if two 1s are same depth we will check above diagonal(i-1,j-1) and take that 
                   if (i > 0 && j > 0 && matrix[i - 1][j - 1] == 1) {
                       best_j = j;  
                       max_count = count;
                   }
               } else {
                   best_j = j;
                   max_count = count;
               }
           }
       }
   }

   // if best_j is -1 then it means row has no 1s
   for (int j = 0; j < n; j++) {
       if (j != best_j) {
           matrix[i][j] = 0; // modify matrix so only best_j is 1 reamining are 0 
       }
   }
   return best_j;  
}

// function for modifying matrix using best1 method
void modfied_matrix(std::vector<std::vector<int>> &matrix, std::vector<int> &index) {
    int m = matrix.size();
    int n = matrix[0].size();
    
    for (int i = 0; i < m; i++) {
         index.push_back(best_index_in_row(matrix, i)); //index vector will store index of best1 in order 
    }
}

// function for finding total length
void finding_patterns(int &i, int &total_length, std::vector<int> &index) {
    if (i >= index.size()) {
        return;   // index out of range
    }
    if (index[i] == -1) { // if row has no 1s we will move to next row i+1
        i = i + 1;
        finding_patterns(i,total_length, index);
    } else {
        
        int count = 1;
        while (true) {
            if (i + count >= index.size()) {
                break; // index out of range
            } else {
                if (index[i + count] == index[i] + count) {
                    count++;
                } else {
                    break;
                }
            }
        }
        if (count >= 10) {
            total_length += count; // we will sum all patterns of length above 10 
        }
        i = i + count; // since every row has only 1 we will directly move to i+count row
        finding_patterns(i,total_length, index);
    }
    return;
}


std::pair<int, std::pair<int, int>> longestCommonSubsequenceLength(const std::vector<int>& vec1, int start1, int end1,
                                                                   const std::vector<int>& vec2, int start2, int end2) {
    if (start1 < 0 || end1 >= vec1.size() || start2 < 0 || end2 >= vec2.size()) {
        return {0, {-1, -1}}; // Return length 0 and indices -1, -1 as indicators of no match
    }

    // Calculate the lengths of the two subarrays
    int m = end1 - start1 + 1;
    int n = end2 - start2 + 1;

    // Create a 2D vector to store the lengths of LCS at each subproblem
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1, 0));

    // Variables to store the indices of the first matching elements
    int firstMatchedIndex1 = -1;
    int firstMatchedIndex2 = -1;
    bool foundFirstMatch = false;

    // Fill dp table using bottom-up dynamic programming
    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            if (vec1[start1 + i - 1] == vec2[start2 + j - 1]) {
                dp[i][j] = dp[i - 1][j - 1] + 1; // Elements match

                // Capture the first matched indices if not already found
                if (!foundFirstMatch) {
                    firstMatchedIndex1 =  i ;
                    firstMatchedIndex2 =  j ;
                    foundFirstMatch = true;
                }
            } else {
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]); // Take the max from previous subproblems
            }
        }
    }

    // Return the LCS length and the indices of the first matching elements
    return {dp[m][n], {firstMatchedIndex1, firstMatchedIndex2}};
}



std::array<int, 5> match_submissions(std::vector<int> &submission1,std::vector<int> &submission2) {
    // TODO: Write your code here
    std::array<int, 5> result = {0, 0, 0, 0, 0};
    int n = submission1.size();
    int m = submission2.size();
    std::vector<std::vector<int>> matrix(m, std::vector<int>(n, 0));

    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            if (submission1[j] == submission2[i]) {
                matrix[i][j] = 1;
            }
        }
    }

    std::vector<int> index;
    modfied_matrix(matrix,index);
    //after modifying single column can have two 1's which will cause repetition,so we will take tranpose and modify again
    std::vector<std::vector<int>> matrix_2(n, std::vector<int>(m, 0));//transpose
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            matrix_2[i][j] = matrix[j][i];
        }
    }
    std::vector<int> index2;//we need another vector(index2) since we are modifying again
    modfied_matrix(matrix_2, index2);
    int start = 0;
    int total_length = 0;
    finding_patterns(start,total_length,index2);

    int p=std::min(n,m);
    if(total_length>=p/2){
        result[0]=1;
    }
    result[1]=total_length;

    
    std::stack<std::pair<int,int>> indices1;
int i = 0;
int l1 = 29;
int j = 0;
n = submission1.size();  // Initialize n and m before the loop
m = submission2.size();
auto lcs = longestCommonSubsequenceLength(submission1, i, i + l1, submission2, j, j + l1);

// Iterate over all possible start indices of submission1 and submission2
while (i <= n - l1) {
    while (j <= m - l1) {
        // Check if the window size is within bounds
        if (i + l1 >= n || j + l1 >= m) break; // Use break instead of continue

        // If the LCS length is greater than 80% of the window size
        if (lcs.first > (4 * (l1 + 1)) / 5) {
            l1 = ((lcs.first * 5) / 4) - 1; // Increase window size proportionally
            if (l1 > std::max(n, m)) {
                l1 = std::max(n, m);
            }
            indices1.push({i, j});
            l1++;
        } else if (lcs.first == (4 * (l1 + 1)) / 5) {
            indices1.push({i, j});
            l1++; // Increment window size by 1
        } else {
            // Move the start indices based on the matched indices to avoid stagnation
           if (lcs.second.second == 0) {
                j++;
            } else if (lcs.second.second == -1) {
                j += l1;
            } else {
                j += lcs.second.second; // Increment j by lcs.second.second
            }
            
        }

        // Check bounds before recalculating LCS
        if (i + l1 >= n || j + l1 >= m) break;

        // Recalculate LCS for the new window
        lcs = longestCommonSubsequenceLength(submission1, i, i + l1, submission2, j, j + l1);
    }
    j = 0;
    i++;
}

// Second part with swapped submissions
std::stack<std::pair<int, int>> indices;
i = 0;
int l = 29;
j = 0;
n = submission2.size();  // Update n and m for the swapped submissions
m = submission1.size();
lcs = longestCommonSubsequenceLength(submission2, i, i + l, submission1, j, j + l);

// Iterate over all possible start indices of submission2 and submission1
while (i <= n - l) {
    while (j <= m - l) {
        // Check if the window size is within bounds
        if (i + l >= n || j + l >= m) break; // Use break instead of continue

        // If the LCS length is greater than 80% of the window size
        if (lcs.first > (4 * (l + 1)) / 5) {
            l = ((lcs.first * 5) / 4) - 1; // Increase window size proportionally
            if (l > std::max(n, m)) {
                l = std::max(n, m);
            }
            indices.push({i, j});
            l++;
        } else if (lcs.first == (4 * (l + 1)) / 5) {
            indices.push({i, j});
            l++; // Increment window size by 1
        } else {
            // Move the start indices based on the matched indices to avoid stagnation
           if (lcs.second.second == 0) {
                j++;
            } else if (lcs.second.second == -1) {
                j += l;
            } else {
                j += lcs.second.second; // Increment j by lcs.second.second
            }
        }

        // Check bounds before recalculating LCS
        if (i + l >= n || j + l >= m) break;

        // Recalculate LCS for the new window
        lcs = longestCommonSubsequenceLength(submission2, i, i + l, submission1, j, j + l);
    }
    j = 0;
    i++;
}

    result[2] = std::max(l,l1)-1;
    if(result[2]==l1){
        if (!indices1.empty()){
            result[3] = indices1.top().first;
            result[4] = indices1.top().second;
        }  
    }
    else{
        if(!indices.empty()){
            result[3] = indices.top().first;
            result[4] = indices.top().second;
        }
    }

    return result;
}   