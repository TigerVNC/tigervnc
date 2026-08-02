package com.jcraft.jsch;

import java.io.IOException;
import java.nio.channels.FileChannel;

public interface FileChannelFactory {
  FileChannel open(String path) throws IOException;
}
