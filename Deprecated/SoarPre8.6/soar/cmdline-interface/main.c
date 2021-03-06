#include "soarapi.h"
#include "soar_core_api.h"
#include "soarkernel.h"
#include "parsing.h"
#include "soarInterfaceCommands.h"
#include "ask.h"
#include <stdio.h>


#ifdef ADD_TCL_RHS_STUB

Symbol *tcl_rhs_function_stub ( list *args ) {	return NIL; }

#endif /* ADD_TCL_RHS_STUB */



/*****************************************************************************
 *
 * The main routine for the shell interface to Soar.
 *
 *****************************************************************************/

int main( int argc, char **argv ) {

  agent *a;
  bool eof_reached;
  int i;
  soarResult res;
  char **temp;


  /*
   *  Initialize Soar, this must be called once (and only once) before
   *  any of Soar's functionality is used.  It basically allocates 
   *  memory and initializes some of Soar's internal data structures
   */
  soar_cInitializeSoar();
  
  /* 
   * Create an agent.  In this example we will only have one agent,
   *  although more are certainly possible
   */
  soar_cCreateAgent( "theAgent" );
  a = soar_cGetAgentByName( "theAgent" );

  /*
   *  Register a function (defined by us) which will handle the output
   *  generated by the kernel.  Most (if not all) interfaces need to
   *  register some type of callback to deal with print calls.  In the
   *  future, it will be easier to avoid this step in the special
   *  circumstances where it is a burden 
   */
  soar_cPushCallback( a, PRINT_CALLBACK, (soar_callback_fn) cb_print,
		     NULL, NULL );

  
  /*
   *  Register a function to handle clean up when Soar is terminated.
   *  This is mainly for illustrative purposes.
   */
  soar_cPushCallback( a, SYSTEM_TERMINATION_CALLBACK, 
		      (soar_callback_fn) cb_exit, NULL, NULL );

  soar_cPushCallback( a, ASK_CALLBACK, 
                      (soar_callback_fn) askCallback, NULL, NULL);
  /*
   *  The interface keeps a table of all of the available commands.
   *  Initialize that table now, before we do anything.
   */
  init_soar_command_table();




#ifdef ADD_TCL_RHS_STUB

  /*
   *  This adds a RHS function named 'tcl' to Soar's vocabulary.  The 
   *  function, however, is simply a null operation (see the definition
   *  above) but it will allow this interface to load Soar productions
   *  which we're built in the TCL environment.  However, whether or not
   *  those applications actually work as intended has a lot to do with
   *  what the real tcl funtion was supposed to do. (Since here it will do
   *  nothing)
   */

  add_rhs_function( make_sym_constant( "tcl" ),
		    tcl_rhs_function_stub, 
		    -1, TRUE, TRUE );
#endif



  /*
   * This small block of code allows Soar to deal with command-line 
   * arguments.  At most one argument is expected.  If such an argument is
   * found, it is taken to be the name of a file which is dealt with as 
   * though the user had typed "source <filename>" at the Soar prompt.
   */
  temp = (char **)malloc( 2 * sizeof( char * ) );
  temp[0] = (char *)malloc( 7 * sizeof( char ) );

  strcpy( temp[0], "source" );
  for( i = 1; i < argc; i++ ) {
   print( "Sourcing '%s'\n", argv[i] );
   init_soarResult( res );
   temp[1] = (char *)malloc( (strlen(argv[i]) + 1) * sizeof( char ) );
   strcpy( temp[1], argv[i] );
   interface_Source( 2, temp, &res );
  } 
    
    
  /*
   * The event loop.  Pretty simple.
   */
  while( 1 ) {
    print( "\nSoar> " );
    executeCommand ( getCommandFromFile( fgetc, stdin, &eof_reached ) );
  }

}







