/////////////////////////////////////////////////////////////////
// rl command file.
//
// Author: Jonathan Voigt, voigtjr@gmail.com, 
//         Nate Derbinsky, nlderbin@umich.edu
// Date  : 2007
//
/////////////////////////////////////////////////////////////////

#include <portability.h>

#include <vector>
#include <map>

#include "cli_CommandLineInterface.h"
#include "cli_Commands.h"

#include "sml_Names.h"
#include "sml_AgentSML.h"

#include "agent.h"

#include "reinforcement_learning.h"
#include "misc.h"

using namespace cli;
using namespace sml;

inline std::string CLI_DoRL_generate_output( const std::string &name, char * const &param )
{
    const std::string output = name + param;
    delete param;
    return output;
}

inline void CLI_DoRL_print( CommandLineInterface &cli, const bool &RawOutput, std::ostringstream &Result,
                           const std::string &text, const bool &newline = true )
{
    if ( RawOutput ) {
        if ( newline )
            Result << text << '\n';
        else
            Result << text;
    }
    else
        cli.AppendArgTagFast( sml_Names::kParamValue, sml_Names::kTypeString, text );
}

/// Formatted for dot: http://www.graphviz.org/content/dot-language

inline void CLI_DoRL_print_trace( std::ostream &os, const agent::RL_Trace &rl_trace, const std::string &label_up = "init", int * cp = NIL )
{
  if(label_up == "init")
    os << "digraph RL_Trace {" << std::endl;

  std::ostringstream label;

  int c = cp ? *cp : 0;
  for(std::map<std::vector<std::string>, agent::RL_Trace::Entry>::const_iterator tt = rl_trace.split.begin(), tend = rl_trace.split.end(); tt != tend; ++tt) {
    label << "node" << c++;

    os << "  " << label.str() << " [label=\"";

    bool sfirst = true;
    for(std::vector<std::string>::const_iterator ss = tt->first.begin(), send = tt->first.end(); ss != send; ++ss) {
      if(sfirst)
        sfirst = false;
      else
        os << "\\n";

      os << *ss;
    }

    os << "\"];" << std::endl;

    os << "  " << label_up << " -> " << label.str() << " [label=\"" << tt->second.probability
       << " (" << tt->second.init << ")\"];" << std::endl;

    if(tt->second.next)
      CLI_DoRL_print_trace(os, *tt->second.next, label.str(), &c);

    label.str("");
    label.clear();
  }

  if(label_up == "init")
    os << "}" << std::endl;

  if(cp)
    *cp = c;
}

bool CommandLineInterface::DoRL( const char pOp, const std::string* pAttr, const std::string* pVal ) 
{
    agent* agnt = m_pAgentSML->GetSoarAgent();
    if ( !pOp )
    {
		CLI_DoRL_print( *this, m_RawOutput, m_Result, "" );
		
		CLI_DoRL_print( *this, m_RawOutput, m_Result,
            CLI_DoRL_generate_output( "Soar-RL learning: ", agnt->rl_params->learning->get_string() ) );

        CLI_DoRL_print( *this, m_RawOutput, m_Result,
            CLI_DoRL_generate_output( "temporal-extension: ", agnt->rl_params->temporal_extension->get_string() ) );

        CLI_DoRL_print( *this, m_RawOutput, m_Result, "" );
        CLI_DoRL_print( *this, m_RawOutput, m_Result, "Discount" );
        CLI_DoRL_print( *this, m_RawOutput, m_Result, "--------" );

        CLI_DoRL_print( *this, m_RawOutput, m_Result,
            CLI_DoRL_generate_output( "discount-rate: ", agnt->rl_params->discount_rate->get_string() ) );

        CLI_DoRL_print( *this, m_RawOutput, m_Result, "" );
        CLI_DoRL_print( *this, m_RawOutput, m_Result, "Learning" );
        CLI_DoRL_print( *this, m_RawOutput, m_Result, "--------" );

        CLI_DoRL_print( *this, m_RawOutput, m_Result,
            CLI_DoRL_generate_output( "learning-policy: ", agnt->rl_params->learning_policy->get_string() ) );

        CLI_DoRL_print( *this, m_RawOutput, m_Result,
            CLI_DoRL_generate_output( "learning-rate: ", agnt->rl_params->learning_rate->get_string() ) );

        CLI_DoRL_print( *this, m_RawOutput, m_Result,
            CLI_DoRL_generate_output( "hrl-discount: ", agnt->rl_params->hrl_discount->get_string() ) );

        CLI_DoRL_print( *this, m_RawOutput, m_Result, "" );
        CLI_DoRL_print( *this, m_RawOutput, m_Result, "Eligibility Traces" );
        CLI_DoRL_print( *this, m_RawOutput, m_Result, "------------------" );

        CLI_DoRL_print( *this, m_RawOutput, m_Result,
            CLI_DoRL_generate_output( "eligibility-trace-decay-rate: ", agnt->rl_params->et_decay_rate->get_string() ) );

        CLI_DoRL_print( *this, m_RawOutput, m_Result,
            CLI_DoRL_generate_output( "eligibility-trace-tolerance: ", agnt->rl_params->et_tolerance->get_string() ) );

        CLI_DoRL_print( *this, m_RawOutput, m_Result, "" );


		CLI_DoRL_print( *this, m_RawOutput, m_Result, "Experimental" );
        CLI_DoRL_print( *this, m_RawOutput, m_Result, "------------" );

		 CLI_DoRL_print( *this, m_RawOutput, m_Result,
            CLI_DoRL_generate_output( "chunk-stop: ", agnt->rl_params->chunk_stop->get_string() ) );

         CLI_DoRL_print( *this, m_RawOutput, m_Result,
            CLI_DoRL_generate_output( "decay-mode: ", agnt->rl_params->decay_mode->get_string() ) );

		 CLI_DoRL_print( *this, m_RawOutput, m_Result,
            CLI_DoRL_generate_output( "meta: ", agnt->rl_params->meta->get_string() ) );

         CLI_DoRL_print( *this, m_RawOutput, m_Result,
            CLI_DoRL_generate_output( "meta-learning-rate: ", agnt->rl_params->meta_learning_rate->get_string() ) );

         CLI_DoRL_print( *this, m_RawOutput, m_Result,
            CLI_DoRL_generate_output( "update-log-path: ", agnt->rl_params->update_log_path->get_string() ) );

		 CLI_DoRL_print( *this, m_RawOutput, m_Result, "" );

		 CLI_DoRL_print( *this, m_RawOutput, m_Result,
            CLI_DoRL_generate_output( "apoptosis: ", agnt->rl_params->apoptosis->get_string() ) );

		 CLI_DoRL_print( *this, m_RawOutput, m_Result,
            CLI_DoRL_generate_output( "apoptosis-decay: ", agnt->rl_params->apoptosis_decay->get_string() ) );

		 CLI_DoRL_print( *this, m_RawOutput, m_Result,
            CLI_DoRL_generate_output( "apoptosis-thresh: ", agnt->rl_params->apoptosis_thresh->get_string() ) );

		 CLI_DoRL_print( *this, m_RawOutput, m_Result, "" );

        return true;
    }
    else if ( pOp == 'g' )
    {
        soar_module::param *my_param = agnt->rl_params->get( pAttr->c_str() );
        if ( !my_param )
            return SetError( "Invalid attribute." );

        CLI_DoRL_print( *this, m_RawOutput, m_Result,
            CLI_DoRL_generate_output( "", my_param->get_string() ), false );

        return true;
    }
    else if ( pOp == 's' )
    {
        soar_module::param *my_param = agnt->rl_params->get( pAttr->c_str() );
        if ( !my_param )
            return SetError( "Invalid attribute." );

        if ( !my_param->validate_string( pVal->c_str() ) )
            return SetError( "Invalid value." );

        if (!my_param->set_string( pVal->c_str() ))
            return SetError( "Failed to set value." );

        return true;
    }
    else if ( pOp == 't' )
    {
      if(pAttr && *pAttr == "clear") {
        agnt->rl_trace.clear();
      }
      else {
        const int level = pAttr ? atoi(pAttr->c_str()) : 1;
        if(level < 1)
          return SetError( "Invalid RL goal level / clear command." );

        std::ostringstream oss;
        
        oss << "# RL Trace, Goal Level " << level << ':' << std::endl;

        std::map<goal_stack_level, agent::RL_Trace>::const_iterator tt = agnt->rl_trace.find(level);
        if(tt != agnt->rl_trace.end())
          CLI_DoRL_print_trace(oss, tt->second);

        CLI_DoRL_print( *this, m_RawOutput, m_Result, oss.str().c_str(), false );
      }

      return true;
    }
    else if ( pOp == 'S' )
    {
        if ( !pAttr )
        {
            CLI_DoRL_print( *this, m_RawOutput, m_Result,
                CLI_DoRL_generate_output( "Error from last update: ", agnt->rl_stats->update_error->get_string() ) );

            CLI_DoRL_print( *this, m_RawOutput, m_Result,
                CLI_DoRL_generate_output( "Total reward in last cycle: ", agnt->rl_stats->total_reward->get_string() ) );

            CLI_DoRL_print( *this, m_RawOutput, m_Result,
                CLI_DoRL_generate_output( "Global reward since init: ", agnt->rl_stats->global_reward->get_string() ) );
        }
        else
        {
            // check attribute name
            soar_module::stat *my_stat = agnt->rl_stats->get( pAttr->c_str() );
            if ( !my_stat )
                return SetError( "Invalid statistic." );

            CLI_DoRL_print( *this, m_RawOutput, m_Result,
                CLI_DoRL_generate_output( "", my_stat->get_string() ), false );
        }

        return true;
    }

    return SetError( "Unknown option.");
}
