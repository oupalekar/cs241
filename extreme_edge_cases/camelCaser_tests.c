/**
 * extreme_edge_cases
 * CS 241 - Spring 2022
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"

int test_camelCaser(char **(*camelCaser)(const char *),
                    void (*destroy)(char **)) {
    // TODO: Implement me!
    char * inputs[] = {"",
                    " ",
                    "./././.",
                    "\t",
                    "\n",
                    "The Heisenbug is an incredible creature. Facenovel servers get their power from its indeterminism. Code smell can be ignored with INCREDIBLE use of air freshener. God objects are the new religion.",
                    "   hi im ojas. im 1.9   ",
                    ".my team is inting me!",
                    "niners. should. have. made. the. super.bowl",
                    "F@ithful to the b@y.",
                    "ABCDEFG fjidfjdsfd.",
                    "123abc, abc123.",
                    "9ffef\xFgde'def.",
                    "3                 a.",
                    NULL};

    char * correct[15][15] =  {
                    {NULL},
                    {"  "},
                    {"","","","","","",""},
                    {""},
                    {""},
                    {"theHeisenbugIsAnIncredibleCreature",
                    "facenovelServersGetTheirPowerFromItsIndeterminism",
                    "codeSmellCanBeIgnoredWithIncredibleUseOfAirFreshener",
                    "godObjectsAreTheNewReligion"},
                    {"hiImOjas", "im1"}, {"", "myTeamIsIntingMe"}, {"niners", "should", "have", "made", "the", "super"},
                    {"f", "ithfulToTheB", "y"}, {"abcdefgFjidfjdsfd"}, {"123abc", "abc123"}, {"9ffef\xFgde", "def"}, {"3A", NULL},
                    {NULL}
    };

    char ** input = inputs;
    int c = 0;
	while(*input){
		int i = 0;
		char **output = (*camelCaser)(*input);
		while(output[i]){
            printf("%s ", output[i]);
            printf("%s\n", correct[c][i]);
			if(strcmp(output[i], correct[c][i])) return 0;
			i++;
		}
        destroy(output);
		c++;
		input++;
	}
    return 1;
}
