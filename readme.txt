Look at:

http://www.bayfronttechnologies.com/mc_tutorial.html


To Compile:
------------------------

	cc metaII_vm.c


To Run:
------------------------

	./a.out

usage: ./a.out [-i #] [-c #] 
-i #		specify example input text index from mc_workshop (i01..i0X)
-c #		specify example code index from mc_workshop       (c01..i0X)
-v  		verbose mode
-l  		list example input and code indexes


To list examples:
------------------------

	./a.out -l

Available Input Texts Are:
	0	i01. demo, AEXP example assignments
	1	i02. demo, AEXP example assignment compiler
	2	i03. Meta II syntax (paper fig. 5)
	3	i04. Meta II syntax (i03 reordered)
	4	i05. add semicolon line end
	5	i06. use semicolon line end
	6	i07. add new output control
	7	i08. delete old output control
	8	i09. use new output control
	9	i10. convert i09 to js functions
	10	i11. add token rules to i09
	11	i11jf. add token rules to i10
	12	i12. use token rules
	13	i12jf. use token rules
	14	i13. add comments and litchr to i12
	15	i13jf. add comments and litchr to i12jf
	16	i14. use comments and litchr
	17	i14jf. use comments and litchr
	18	i14js. convert i14jf to js object
	19	i02a. demo, AEXP example assignment compiler
	20	i01a. demo, AEXP2 backup value assignments
	21	i15js. js metacompiler with backup
	22	i02b. demo, AEXP2 backup assignment compiler
	23	i04a. Meta II syntax (i03 reordered and tokens)


Available Codes Are:
	0	c00. demo, compiled assignments c[i01,c01] 
	1	c01. demo, AEXP assignments compiler c[i02,c02] 
	2	c02. Meta II of fig. 5, m[i03,c02]
	3	c03. Meta II reordered, c[i04,c02], m[i04,c03]
	4	c04. Meta II semicolons, c[i05,c03], m[i06,c04]
	5	c05. accept new output, c[i07,c04], m[i07,c05]
	6	c06. reject old output, c[i08,c05]
	7	c07. use new output, c[i09,c06], m[i09,c07]
	8	c08. compile to js functions, c[i10,c07]
	9	c09. accept tokens, c[i11,c07]
	10	c10. use tokens, c[i12,c09], m[i12,c10]
	11	c11. accept comments, c[i13,c10]
	12	c12. use comments, c[i14,c11], m[i14,c12]



Test Run:
------------------------

	./a.out -i 2 -c 2


	ADR PROGRAM
OUT1
	TST '*1'
	BF L1
	CL 'GN1'
	OUT
L1
	BT L2
	TST '*2'
	BF L3
	CL 'GN2'
	OUT
L3
	BT L2
	TST '*'
	BF L4
	CL 'CI'
	OUT
L4
	BT L2
	SR
	BF L5
	CL 'CL '
	CI
	OUT
L5
L2
	R
OUTPUT
	TST '.OUT'
	BF L6
	TST '('
	BE
L7
	CLL OUT1
	BT L7
	SET
	BE
	TST ')'
	BE
L6
	BT L8
	TST '.LABEL'
	BF L9
	CL 'LB'
	OUT
	CLL OUT1
	BE
L9
L8
	BF L10
	CL 'OUT'
	OUT
L10
L11
	R
EX3
	ID
	BF L12
	CL 'CLL '
	CI
	OUT
L12
	BT L13
	SR
	BF L14
	CL 'TST '
	CI
	OUT
L14
	BT L13
	TST '.ID'
	BF L15
	CL 'ID'
	OUT
L15
	BT L13
	TST '.NUMBER'
	BF L16
	CL 'NUM'
	OUT
L16
	BT L13
	TST '.STRING'
	BF L17
	CL 'SR'
	OUT
L17
	BT L13
	TST '('
	BF L18
	CLL EX1
	BE
	TST ')'
	BE
L18
	BT L13
	TST '.EMPTY'
	BF L19
	CL 'SET'
	OUT
L19
	BT L13
	TST '$'
	BF L20
	LB
	GN1
	OUT
	CLL EX3
	BE
	CL 'BT '
	GN1
	OUT
	CL 'SET'
	OUT
L20
L13
	R
EX2
	CLL EX3
	BF L21
	CL 'BF '
	GN1
	OUT
L21
	BT L22
	CLL OUTPUT
	BF L23
L23
L22
	BF L24
L25
	CLL EX3
	BF L26
	CL 'BE'
	OUT
L26
	BT L27
	CLL OUTPUT
	BF L28
L28
L27
	BT L25
	SET
	BE
	LB
	GN1
	OUT
L24
L29
	R
EX1
	CLL EX2
	BF L30
L31
	TST '/'
	BF L32
	CL 'BT '
	GN1
	OUT
	CLL EX2
	BE
L32
L33
	BT L31
	SET
	BE
	LB
	GN1
	OUT
L30
L34
	R
ST
	ID
	BF L35
	LB
	CI
	OUT
	TST '='
	BE
	CLL EX1
	BE
	TST '.,'
	BE
	CL 'R'
	OUT
L35
L36
	R
PROGRAM
	TST '.SYNTAX'
	BF L37
	ID
	BE
	CL 'ADR '
	CI
	OUT
L38
	CLL ST
	BT L38
	SET
	BE
	TST '.END'
	BE
	CL 'END'
	OUT
L37
L39
	R
	END












