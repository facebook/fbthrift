/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

using System;
using System.IO;

namespace Thrift.Transport
{
	public class TStreamTransport : TTransport
	{
		protected Stream inputStream;
		protected Stream outputStream;

		public TStreamTransport()
		{
		}

		public TStreamTransport(Stream inputStream, Stream outputStream)
		{
			this.inputStream = inputStream;
			this.outputStream = outputStream;
		}

		public Stream OutputStream
		{
			get { return outputStream; }
		}

		public Stream InputStream
		{
			get { return inputStream; }
		}

		public override bool IsOpen
		{
			get { return true; }
		}

		public override void Open()
		{
		}

		public override void Close()
		{
			if (inputStream != null)
			{
				inputStream.Close();
				inputStream = null;
			}
			if (outputStream != null)
			{
				outputStream.Close();
				outputStream = null;
			}
		}

		public override int Read(byte[] buf, int off, int len)
		{
			if (inputStream == null)
			{
				throw new TTransportException(TTransportException.ExceptionType.NotOpen, "Cannot read from null inputstream");
			}

			return inputStream.Read(buf, off, len);
		}

		public override void Write(byte[] buf, int off, int len)
		{
			if (outputStream == null)
			{
				throw new TTransportException(TTransportException.ExceptionType.NotOpen, "Cannot write to null outputstream");
			}

			outputStream.Write(buf, off, len);
		}

		public override void Flush()
		{
			if (outputStream == null)
			{
				throw new TTransportException(TTransportException.ExceptionType.NotOpen, "Cannot flush null outputstream");
			}

			outputStream.Flush();
		}
	}
}
