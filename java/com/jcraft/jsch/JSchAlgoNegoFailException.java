package com.jcraft.jsch;

import java.util.Locale;

/**
 * Extension of {@link JSchException} to indicate when a connection fails during algorithm
 * negotiation.
 */
public class JSchAlgoNegoFailException extends JSchException {

  private static final long serialVersionUID = -1L;

  private final String algorithmName;
  private final String jschProposal;
  private final String serverProposal;

  JSchAlgoNegoFailException(int algorithmIndex, String jschProposal, String serverProposal) {
    super(failString(algorithmIndex, jschProposal, serverProposal));
    algorithmName = algorithmNameFromIndex(algorithmIndex);
    this.jschProposal = jschProposal;
    this.serverProposal = serverProposal;
  }

  /** Get the algorithm name. */
  public String getAlgorithmName() {
    return algorithmName;
  }

  /** Get the JSch algorithm proposal. */
  public String getJSchProposal() {
    return jschProposal;
  }

  /** Get the server algorithm proposal. */
  public String getServerProposal() {
    return serverProposal;
  }

  private static String failString(int algorithmIndex, String jschProposal, String serverProposal) {
    return String.format(Locale.ROOT,
        "Algorithm negotiation fail: algorithmName=\"%s\" jschProposal=\"%s\" serverProposal=\"%s\"",
        algorithmNameFromIndex(algorithmIndex), jschProposal, serverProposal);
  }

  private static String algorithmNameFromIndex(int algorithmIndex) {
    switch (algorithmIndex) {
      case KeyExchange.PROPOSAL_KEX_ALGS:
        return "kex";
      case KeyExchange.PROPOSAL_SERVER_HOST_KEY_ALGS:
        return "server_host_key";
      case KeyExchange.PROPOSAL_ENC_ALGS_CTOS:
        return "cipher.c2s";
      case KeyExchange.PROPOSAL_ENC_ALGS_STOC:
        return "cipher.s2c";
      case KeyExchange.PROPOSAL_MAC_ALGS_CTOS:
        return "mac.c2s";
      case KeyExchange.PROPOSAL_MAC_ALGS_STOC:
        return "mac.s2c";
      case KeyExchange.PROPOSAL_COMP_ALGS_CTOS:
        return "compression.c2s";
      case KeyExchange.PROPOSAL_COMP_ALGS_STOC:
        return "compression.s2c";
      case KeyExchange.PROPOSAL_LANG_CTOS:
        return "lang.c2s";
      case KeyExchange.PROPOSAL_LANG_STOC:
        return "lang.s2c";
      default:
        return "";
    }
  }
}
