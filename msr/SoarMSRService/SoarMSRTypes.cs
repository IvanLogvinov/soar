//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version:2.0.50727.1433
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

using Microsoft.Ccr.Core;
using Microsoft.Dss.Core.Attributes;
using Microsoft.Dss.ServiceModel.Dssp;
using System;
using System.Collections.Generic;
using W3C.Soap;
using soarmsr = Robotics.SoarMSR;


namespace Robotics.SoarMSR
{
    
    
    /// <summary>
    /// SoarMSR Contract class
    /// </summary>
    public sealed class Contract
    {
        
        /// <summary>
        /// The Dss Service contract
        /// </summary>
        public const String Identifier = "http://schemas.tempuri.org/2008/03/soarmsrservice.html";
    }
    
    /// <summary>
    /// The SoarMSR State
    /// </summary>
    [DataContract()]
    public class SoarMSRState
    {
        // True to spawn the debugger on agent creation
        [DataMember]
        public bool SpawnDebugger = true;
    }
    
    /// <summary>
    /// SoarMSR Main Operations Port
    /// </summary>
    [ServicePort()]
    public class SoarMSROperations : PortSet<DsspDefaultLookup, DsspDefaultDrop, Get, Replace>
    {
    }
    
    /// <summary>
    /// SoarMSR Get Operation
    /// </summary>
    public class Get : Get<GetRequestType, PortSet<SoarMSRState, Fault>>
    {
        
        /// <summary>
        /// SoarMSR Get Operation
        /// </summary>
        public Get()
        {
        }
        
        /// <summary>
        /// SoarMSR Get Operation
        /// </summary>
        public Get(Microsoft.Dss.ServiceModel.Dssp.GetRequestType body) : 
                base(body)
        {
        }
        
        /// <summary>
        /// SoarMSR Get Operation
        /// </summary>
        public Get(Microsoft.Dss.ServiceModel.Dssp.GetRequestType body, Microsoft.Ccr.Core.PortSet<SoarMSRState,W3C.Soap.Fault> responsePort) : 
                base(body, responsePort)
        {
        }
    }

    public class Replace : Replace<SoarMSRState, PortSet<DefaultReplaceResponseType, Fault>>
    {
    }
}
