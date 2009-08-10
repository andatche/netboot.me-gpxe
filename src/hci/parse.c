#include <gpxe/parse.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gpxe/settings.h>

int incomplete;

/* Table for arithmetic expressions */
const struct char_table arith_table[22] = {
	{
		.token = '\\',
		.type = FUNC,
		.next.parse_func = parse_escape
	},
	{
		.token = '"',
		.type = TABLE,
		.next = {
			.next_table = {
				.ntable = dquote_table,
				.len = 3
			}
		}
	},
	{
		.token = '$',
		.type = FUNC,
		.next.parse_func = dollar_expand
	},
	{
		.token = '\'',
		.type = TABLE,
		.next = {
			.next_table = {
				.ntable = squote_table,
				.len = 1
			}
		}
	},
	{
		.token = ' ',
		.type = ENDQUOTES
	},
	{
		.token = '\t',
		.type = ENDQUOTES
	},
	{
		.token = '\n',
		.type = ENDQUOTES
	},
	{
		.token = '~',
		.type = ENDTOK
	},
	{
		.token = '!',
		.type = ENDTOK
	},
	{
		.token = '*',
		.type = ENDTOK
	},
	{
		.token = '/',
		.type = ENDTOK
	},
	{
		.token = '%',
		.type = ENDTOK
	},
	{
		.token = '+',
		.type = ENDTOK
	},
	{
		.token = '-',
		.type = ENDTOK
	},
	{
		.token = '<',
		.type = ENDTOK
	},
	{
		.token = '=',
		.type = ENDTOK
	},
	{
		.token = '>',
		.type = ENDTOK
	},
	{
		.token = '&',
		.type = ENDTOK
	},
	{
		.token = '|',
		.type = ENDTOK
	},
	{
		.token = '^',
		.type = ENDTOK
	},
	{
		.token = '(',
		.type = ENDTOK
	},
	{
		.token = ')',
		.type = ENDTOK
	}
};

/* Table for parsing text in double-quotes */
const struct char_table dquote_table[3] = {
	{
		.token = '"',
		.type = ENDQUOTES
	},
	{
		.token = '$', .type = FUNC,
		.next.parse_func = dollar_expand
	},
	{
		.token = '\\',
		.type = FUNC,
		.next.parse_func = parse_escape
	}
};

/* Table to parse text in single-quotes */
const struct char_table squote_table[1] = {
	{
		.token = '\'',
		.type = ENDQUOTES
	}
};

/* Table to start with */
const struct char_table table[6] = {
	{
		.token = '\\',
		.type = FUNC,
		.next.parse_func = parse_escape
	},
	{
		.token = '"',
		.type = TABLE,
		.next = {
			.next_table = {
				.ntable = dquote_table,
				.len = 3
			}
		}
	},
	{
		.token = '$',
		.type = FUNC,
		.next.parse_func = dollar_expand
	},
	{
		.token = '\'',
		.type = TABLE,
		.next = {
			.next_table = {
				.ntable = squote_table,
				.len = 1
			}
		}
	},
	{
		.token = ' ',
		.type = ENDTOK
	},
	{
		.token = '\t',
		.type = ENDTOK
	}
};

/* Table to parse text after a $ */
const struct char_table dollar_table[2] = {
	{
		.token = '}',
		.type = ENDTOK
	},
	{
		.token = '$',
		.type = FUNC,
		.next.parse_func = dollar_expand
	},
};

/** Function to combine a struct string with 2 char * */
char * string3cat ( struct string *s1, const char *s2,
	const char *s3 ) {
		
	char *tmp = s1->value;
		
	if ( s1->value )
		asprintf ( &s1->value, "%s%s%s", s1->value, s2, s3 );
	else {
		stringcpy ( s1, s2 );
		if ( s1->value )
			stringcat ( s1, s3 );
	}
	free ( tmp );
	return s1->value;
}

/** Function to copy a char * into a struct string */
char * stringcpy ( struct string *s1, const char *s2 ) {
	char *tmp = s1->value;
	s1->value = strdup ( s2 );
	free ( tmp );
	return s1->value;
}

/** Function to concatenate a struct string and a char * */
char * stringcat ( struct string *s1, const char *s2 ) {
	char *tmp;
	if ( !s1->value )
		return stringcpy ( s1, s2 );
	if ( !s2 )
		return s1->value;
	tmp = s1->value;
	asprintf ( &s1->value, "%s%s", s1->value, s2 );
	free ( tmp );
	return s1->value;
}

/** Free a struct string */
void free_string ( struct string *s ) {
	free ( s->value );
	s->value = NULL;
}

/** Expand the text following a $ */
char * dollar_expand ( struct string *s, char *inp ) {
	char *name;
	int setting_len;
	int len;
	char *end;
	
	len = inp - s->value;

	if ( inp[1] == '{' ) {
		end = expand_string ( s, inp + 2, dollar_table,
			2, 1 );
		inp = s->value + len;
		if ( end ) {
			*inp = 0;
			*end++ = 0;
			name = inp + 2;
			/* Determine setting length */
			setting_len = fetchf_named_setting ( name, NULL, 0 );
			if ( setting_len < 0 )
				setting_len = 0; /* Treat error as empty setting */
			
			/* Read setting into buffer */
			{
				char expdollar[setting_len + 1];
				expdollar[0] = '\0';
				fetchf_named_setting ( name, expdollar,
								setting_len + 1 );
				if ( string3cat ( s, expdollar, end ) )
					end = s->value + len + strlen ( expdollar );
			}
		}
		return end;
	} else if ( inp[1] == '(' ) {
		name = inp;
		{
			end = parse_arith ( s, name );
			return end;
		}
	}
	/* Can't find { or (, so preserve the $ */
	end = inp + 1;
	return end;
}

/** Deal with an escape sequence */
char * parse_escape ( struct string *s, char *input ) {
	char *exp;
	char *end;
	
	/* Found a \ at the end of the string => more input required */
	if ( ! input[1] ) {
		incomplete = 1;
		free_string ( s );
		return NULL;
	}
	*input = 0;
	end = input + 2;
	if ( input[1] == '\n' ) {
		/* For a \ at end of line, remove both \ and \n */
		int len = input - s->value;
		exp = stringcat ( s, end );
		end = exp + len;
	} else {
		/* A \ removes the special meanig of any other character */
		int len = input - s->value;
		end = input + 1;
		exp = stringcat ( s, end );
		end = exp + len + 1;
	}
	return end;
}

/* Return a pointer to the first unconsumed character */
char * expand_string ( struct string *s, char *head,
	const struct char_table *table, int tlen,
	int in_quotes ) {
	
	int i;
	int cur_pos; /* s->value may be reallocated */

	cur_pos = head - s->value;
	
	while ( s->value[cur_pos] ) {
		const struct char_table * tline = NULL;
		
		for ( i = 0; i < tlen; i++ ) {
			/* Look for current token in the list we have */
			if ( table[i].token == s->value[cur_pos] ) {
				tline = table + i;
				break;
			}
		}
		
		if ( ! tline ) { /* If not found, copy into output */
			cur_pos++;
		} else {
			switch ( tline->type ) {
				case ENDQUOTES:
				/* End of input, where next char is to be discarded.
				 * Used for ending ' or " */
				s->value[cur_pos] = 0;
				if ( ! stringcat ( s, s->value + cur_pos + 1 ) )
					return NULL;
				return s->value + cur_pos;
				
				case TABLE:
				{
				/* Recursive call. Probably found quotes */
					char *tmp;

					s->value[cur_pos] = 0;
					/* Remove the current character and
					 * call recursively */
					if ( !stringcat ( s, s->value + cur_pos + 1 ) )
						return NULL;
					
					tmp = expand_string ( s, s->value + cur_pos, tline->next.next_table.ntable, tline->next.next_table.len, 1 );
					if ( ! tmp )
						return NULL;
					cur_pos = tmp - s->value;
					break;
				}
				case FUNC: /* Call another function */
					{
						char *tmp;
						if ( ! ( tmp = tline->next.parse_func
							( s, s->value + cur_pos ) ) )
							return NULL;
						cur_pos = tmp - s->value;
					}
					break;
					
				case ENDTOK:
					/* End of input, and we also want next
					 * character */
					return s->value + cur_pos;
					break;
			}
		}
		
	}
	if ( in_quotes ) {
		/* We haven't found the closing quotes */
		incomplete = 1;
		free_string ( s );
		return NULL;
	}
	return s->value + cur_pos;
}
