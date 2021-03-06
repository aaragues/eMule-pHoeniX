%{
#include "stdafx.h"
#include "resource.h"
#include "SearchExpr.h"
#include "scanner.h"
#include "OtherFunctions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CStringArray _astrParserErrors;

void ParsedSearchExpression(const CSearchExpr* pexpr);
int yyerror(const char* errstr);
#ifdef _UNICODE
int yyerror(LPCTSTR errstr);
#endif

#pragma warning(disable:4065) // switch statement contains 'default' but no 'case' labels
#pragma warning(disable:4102) // 'yyerrlab1' : unreferenced label

%}

%union {
	CStringA*		pstr;
	CSearchExpr*	pexpr;
}

%token TOK_STRING
%token TOK_AND TOK_OR TOK_NOT
%token TOK_ED2K_LINK

%type <pexpr> searchexpr and_strings
%type <pstr> TOK_STRING TOK_ED2K_LINK

%left TOK_OR
%left TOK_AND
%left TOK_NOT

%%
/*-------------------------------------------------------------------*/

action			: searchexpr
					{
						ParsedSearchExpression($1);
						delete $1;
						return 0;
					}
				| TOK_ED2K_LINK
					{
						CSearchExpr* pexpr = new CSearchExpr($1);
						ParsedSearchExpression(pexpr);
						delete pexpr;
						delete $1;
						return 0;
					}
				/* --------- Error Handling --------- */
				| searchexpr error
					{
						yyerror(GetResString(IDS_SEARCH_GENERALERROR));
						delete $1;
						return 1;
					}
				;


searchexpr		: and_strings
				| searchexpr TOK_AND searchexpr
					{
						CSearchExpr* pexpr = new CSearchExpr;
						pexpr->Add(SEARCHOP_AND);
						pexpr->Add($1);
						pexpr->Add($3);
						$$ = pexpr;
						delete $1;
						delete $3;
					}
				| searchexpr TOK_OR searchexpr
					{
						CSearchExpr* pexpr = new CSearchExpr;
						pexpr->Add(SEARCHOP_OR);
						pexpr->Add($1);
						pexpr->Add($3);
						$$ = pexpr;
						delete $1;
						delete $3;
					}
				| searchexpr TOK_NOT searchexpr
					{
						CSearchExpr* pexpr = new CSearchExpr;
						pexpr->Add(SEARCHOP_NOT);
						pexpr->Add($1);
						pexpr->Add($3);
						$$ = pexpr;
						delete $1;
						delete $3;
					}
				| '(' searchexpr ')'
					{
						$$ = $2;
					}
				/* --------- Error Handling --------- */
				| searchexpr TOK_OR error
					{
						yyerror(GetResString(IDS_SEARCH_MISSINGORRIGHT));
						delete $1;
						return 1;
					}
				| searchexpr TOK_NOT error
					{
						yyerror(GetResString(IDS_SEARCH_MISSINGNOTRIGHT));
						delete $1;
						return 1;
					}
				| '(' error
					{
						yyerror(GetResString(IDS_SEARCH_MISSINGEXPRPARANT));
						return 1;
					}
				| '(' searchexpr error
					{
						yyerror(GetResString(IDS_SEARCH_MISSINGCLOSINGPARANT));
						delete $2;
						return 1;
					}
				| TOK_AND error
					{
						yyerror(GetResString(IDS_SEARCH_MISSINGANDLEFT));
						return 1;
					}
				| TOK_OR error
					{
						yyerror(GetResString(IDS_SEARCH_MISSINGORLEFT));
						return 1;
					}
				| TOK_NOT error
					{
						yyerror(GetResString(IDS_SEARCH_MISSINGNOTLEFT));
						return 1;
					}
				;

and_strings		: TOK_STRING
					{
						$$ = new CSearchExpr($1);
						delete $1;
					}
				| and_strings TOK_STRING
					{
						/*$1->Concatenate($2);
						delete $2;*/
						CSearchExpr* pexpr = new CSearchExpr;
						pexpr->Add(SEARCHOP_AND);
						pexpr->Add($1);
						pexpr->Add($2);
						$$ = pexpr;
						delete $1;
						delete $2;
					}
				;

%%

int yyerror(const char* errstr)
{
	// Errors created by yacc generated code
	//yyerror ("syntax error: cannot back up");
	//yyerror ("syntax error; also virtual memory exhausted");
	//yyerror ("syntax error");
	//yyerror ("parser stack overflow");

	USES_CONVERSION;
	_astrParserErrors.Add(A2CT(errstr));
	return EXIT_FAILURE;
}

#ifdef _UNICODE
int yyerror(LPCTSTR errstr)
{
	_astrParserErrors.Add(errstr);
	return EXIT_FAILURE;
}
#endif
