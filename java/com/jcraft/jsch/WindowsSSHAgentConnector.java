package com.jcraft.jsch;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;

public class WindowsSSHAgentConnector implements AgentConnector {
  private static final String DEFAULT_OPENSSH_AGENT_NAMED_PIPE = "\\\\.\\pipe\\openssh-ssh-agent";
  private static final int MAX_AGENT_REPLY_LEN = 256 * 1024;

  private final FileChannelFactory fileChannelFactory;
  private final String sshAgentNamedPipe;


  public WindowsSSHAgentConnector() {
    this(path -> new RandomAccessFile(path, "rw").getChannel(), DEFAULT_OPENSSH_AGENT_NAMED_PIPE);
  }

  public WindowsSSHAgentConnector(String sshAgentNamedPipe) {
    this(path -> new RandomAccessFile(path, "rw").getChannel(), sshAgentNamedPipe);
  }

  public WindowsSSHAgentConnector(FileChannelFactory fileChannelFactory) {
    this(fileChannelFactory, DEFAULT_OPENSSH_AGENT_NAMED_PIPE);
  }

  public WindowsSSHAgentConnector(FileChannelFactory fileChannelFactory, String sshAgentNamedPipe) {
    this.fileChannelFactory = fileChannelFactory;
    this.sshAgentNamedPipe = sshAgentNamedPipe;
  }

  @Override
  public String getName() {
    return "ssh-agent - Windows Named Pipe";
  }

  @Override
  public boolean isAvailable() {
    try (FileChannel fileChannel = open()) {
      return fileChannel.isOpen();
    } catch (IOException e) {
      return false;
    }
  }

  @Override
  public void query(Buffer buffer) throws AgentProxyException {
    try (FileChannel fileChannel = open()) {
      // writes request
      writeFull(fileChannel, buffer, 0, buffer.getLength());

      // read length of reply
      buffer.rewind();
      int i = readFull(fileChannel, buffer, 0, 4); // length
      i = buffer.getInt();
      if (i <= 0 || i > MAX_AGENT_REPLY_LEN) {
        throw new AgentProxyException("Illegal length: " + i);
      }

      // read reply
      buffer.rewind();
      buffer.checkFreeSize(i);
      i = readFull(fileChannel, buffer, 0, i);
    } catch (IOException e) {
      throw new AgentProxyException("I/O error communicating with OpenSSH agent: " + e.getMessage(),
          e);
    }
  }

  private FileChannel open() throws IOException {
    return fileChannelFactory.open(sshAgentNamedPipe);
  }

  private static int readFull(FileChannel fileChannel, Buffer buffer, int s, int len)
      throws IOException {
    ByteBuffer bb = ByteBuffer.wrap(buffer.buffer, s, len);
    int _len = len;
    while (len > 0) {
      int j = fileChannel.read(bb);
      if (j < 0)
        return -1;
      if (j > 0) {
        len -= j;
      }
    }
    return _len;
  }

  private static int writeFull(FileChannel fileChannel, Buffer buffer, int s, int len)
      throws IOException {
    ByteBuffer bb = ByteBuffer.wrap(buffer.buffer, s, len);
    int _len = len;
    while (len > 0) {
      int j = fileChannel.write(bb);
      if (j < 0)
        return -1;
      if (j > 0) {
        len -= j;
      }
    }
    return _len;
  }
}
