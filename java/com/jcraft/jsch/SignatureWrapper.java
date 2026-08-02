package com.jcraft.jsch;

/**
 * A factory and wrapper class for creating and managing digital signature instances.
 * <p>
 * This class abstracts the creation of specific signature algorithm implementations (like RSA, DSA,
 * ECDSA, EdDSA) by dynamically loading them based on an algorithm name. It also provides a generic
 * interface for setting the public key and performing signature operations (init, update, verify,
 * sign) by delegating calls to the underlying signature instance.
 * </p>
 */
class SignatureWrapper implements Signature {

  private final Signature signature;

  private final PubKeySetter publicKeySetter;

  private final PubKeyParameterValidator pubKeyParameterValidator;

  /**
   * Constructs a {@code SignatureWrapper} for the specified signature algorithm.
   * <p>
   * This constructor uses reflection to find and instantiate the appropriate {@link Signature}
   * implementation based on the algorithm name retrieved from JSch's configuration. It also sets up
   * handlers for public key validation and setting based on the algorithm type.
   * </p>
   *
   * @param algorithm the name of the signature algorithm (e.g., "ssh-rsa", "ssh-dss").
   * @throws JSchException if the specified algorithm is not recognized, the implementation class
   *         cannot be found, or an instance cannot be created.
   */
  SignatureWrapper(String algorithm, Session session) throws JSchException {
    try {
      // Session.getConfig(algorithm)
      this.signature = Class.forName(session.getConfig(algorithm)).asSubclass(Signature.class)
          .getDeclaredConstructor().newInstance();
    } catch (Exception | LinkageError e) {
      throw new JSchException("Failed to instantiate signature for algorithm '" + algorithm + "'",
          e);
    }

    if (signature instanceof SignatureRSA) {
      pubKeyParameterValidator = (byte[][] args) -> generateValidator("RSA", 2);
      publicKeySetter =
          (byte[][] args) -> ((SignatureRSA) this.signature).setPubKey(args[0], args[1]);
    } else if (signature instanceof SignatureDSA) {
      pubKeyParameterValidator = (byte[][] args) -> generateValidator("DSA", 4);
      publicKeySetter = (byte[][] args) -> ((SignatureDSA) this.signature).setPubKey(args[0],
          args[1], args[2], args[3]);
    } else if (signature instanceof SignatureECDSA) {
      pubKeyParameterValidator = (byte[][] args) -> generateValidator("ECDSA", 2);
      publicKeySetter =
          (byte[][] args) -> ((SignatureECDSA) this.signature).setPubKey(args[0], args[1]);
    } else if (signature instanceof SignatureEdDSA) {
      pubKeyParameterValidator = (byte[][] args) -> generateValidator("EdDSA", 1);
      publicKeySetter = (byte[][] args) -> ((SignatureEdDSA) this.signature).setPubKey(args[0]);
    } else {
      throw new JSchException("Unrecognized signature algorithm: " + algorithm);
    }
  }

  /**
   * Generates a validator for the public key parameters.
   *
   * @param algorithm the algorithm name, used for error messages.
   * @param expectedNumParams the exact number of byte arrays expected for the public key.
   * @return a {@link PubKeyParameterValidator} instance.
   * @throws JSchException if the number of provided parameters does not match the expected count.
   */
  private static PubKeyParameterValidator generateValidator(String algorithm, int expectedNumParams)
      throws JSchException {
    return (byte[][] params) -> {
      if (params.length != expectedNumParams) {
        throw new JSchException("wrong number of arguments:" + algorithm + " signatures expects "
            + expectedNumParams + " parameters, found " + params.length);
      }
    };
  }

  /**
   * Initializes the underlying signature instance for signing or verification. This method
   * delegates the call to the wrapped signature object.
   *
   * @throws Exception if an error occurs during initialization.
   */
  @Override
  public void init() throws Exception {
    signature.init();
  }

  /**
   * Updates the data to be signed or verified with the given byte array. This method delegates the
   * call to the wrapped signature object.
   *
   * @param H the byte array to update the signature data with.
   * @throws Exception if an error occurs during the update.
   */
  @Override
  public void update(byte[] H) throws Exception {
    signature.update(H);
  }

  /**
   * Verifies the provided signature. This method delegates the call to the wrapped signature
   * object.
   *
   * @param sig the signature bytes to be verified.
   * @return {@code true} if the signature is valid, {@code false} otherwise.
   * @throws Exception if an error occurs during verification.
   */
  @Override
  public boolean verify(byte[] sig) throws Exception {
    return signature.verify(sig);
  }

  /**
   * Generates the digital signature of all the data updated so far. This method delegates the call
   * to the wrapped signature object.
   *
   * @return the byte array representing the digital signature.
   * @throws Exception if an error occurs during the signing process.
   */
  @Override
  public byte[] sign() throws Exception {
    return signature.sign();
  }

  /**
   * Sets the public key required for signature verification.
   * <p>
   * This method first validates that the correct number of key parameters are provided for the
   * specific algorithm and then passes them to the underlying signature instance.
   * </p>
   *
   * @param args a variable number of byte arrays representing the public key components.
   * @throws Exception if the key parameters are invalid or if an error occurs while setting the
   *         public key on the underlying signature instance.
   */
  void setPubKey(byte[]... args) throws Exception {
    pubKeyParameterValidator.validatePublicKeyParameter(args);
    publicKeySetter.setPubKey(args);
  }

  /**
   * A functional interface for setting a public key on a {@link Signature} instance.
   */
  @FunctionalInterface
  private interface PubKeySetter {

    /**
     * Sets the public key components on the signature instance.
     *
     * @param keyParams the components of the public key.
     * @throws Exception if an error occurs during the operation.
     */
    void setPubKey(byte[]... keyParams) throws Exception;
  }

  /**
   * A functional interface for validating the public key parameters.
   */
  @FunctionalInterface
  private interface PubKeyParameterValidator {
    /**
     * Validates the provided public key components.
     *
     * @param keyParams the components of the public key to validate.
     * @throws Exception if the parameters are invalid.
     */
    void validatePublicKeyParameter(byte[]... keyParams) throws Exception;
  }

}
