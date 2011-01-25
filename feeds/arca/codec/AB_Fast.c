#include <stdlib.h>
#include <stdio.h>
//#include <winsock2.h>
#include <string.h>
#include <time.h>

/* defines for message and standard types */
#define INSTANTIATE_FASTSTATEINIT
#include "AB_Fast.h"

/* Define some cross platform CPU measurement utilities */

/* Decode the Arca Multicast
*/

/*
* Test harness
*/

#define READ_LEN (1024)
#define FRAME_SIZE (1500)
int main(int argc, char* argv[])
{
	FILE *fptrIn;
	FILE *fptrOut;
	int32_t iCount  = 0;                /* total messages processed */
	uint32_t packSize = 0;
	uint32_t unpackSize = 0;
	// uint32_t frameLen = 0;
    // long long start_cpu_time,end_cpu_time;
	FAST_STATE state[AB_MAX_FIELD];
	uint8_t fastBuf[READ_LEN*2];
	uint8_t tmpBuf[READ_LEN*2];
	int32_t len /* ,msgSize */;
	int32_t mode = 0;
    ArcaL2MsgUnion  msg;
    int             clkStatus ;
    struct timespec entryCpuClock = { 0, 0 };
    struct timespec exitCpuClock  = { 0, 0 } ;
    struct timespec cumCpuClock   = { 0, 0 } ;
	/* the current binary message we are working on */
    	/*PacketHeader_t*/ArcaL2Header_t  packetHeader;  
	/* the current packet header we are working on */
	memset(&msg,0,sizeof(msg));
	memcpy(&state,fastStateInit,sizeof(fastStateInit));

	if(argc < 4 || (strcmp(argv[1],"-d") && strcmp(argv[1],"-e")))
	{
		fprintf(stderr,"ARCA Book Options to ARCA Book Options FAST converter\n");
		fprintf(stderr,"Usage: %s mode inputFile outputFile\n",argv[0]);
		fprintf(stderr,"Where mode is -e to encode, -d to decode\n");
		fprintf(stderr,"Example: %s -e ArcaBinary.dat ArcaBinary.fast\n",argv[0]);
		fprintf(stderr,"Example: %s -d ArcaBinary.fast ArcaBinary2.dat\n",argv[0]);

		return 1;
	}

	if(0==strcmp(argv[1],"-e"))
		mode = 1;
	else if(0==strcmp(argv[1],"-d"))
		mode = 2;
	else
	{
		fprintf(stderr,"Unknown mode %s\n",argv[1]);
		return 1;
	}

	/* open both the input and output files */
	fptrIn = fopen(argv[2],"rb");
	if(NULL==fptrIn)
	{
		fprintf(stderr,"Can not open input file %s\n",argv[2]);
		return 1;
	}

	fptrOut = fopen(argv[3],"wb");
	if(NULL==fptrOut)
	{
		fprintf(stderr,"Can not open output file %s\n",argv[3]);
		fclose(fptrIn);
		return 1;
	}

	/* Both files open, ready to roll ... */
	fprintf(stderr,"%s %s to %s\n",mode==1 ? "Encoding":"Decoding ",argv[2],argv[3]);

	//start_cpu_time = get_cpu_time();
	/* compact the data */
	if(1==mode)
	{
		fprintf(stderr,"Encode Option not available.\n");
	}
	else /* Uncompact the data */
	{
		int32_t iBufPos;   // current position within the input buffer we are reading
		int32_t iBufLen;   // the amount of data left in the current read buffer
		int32_t iReadLen;
		int32_t tmpLen = 0; 
		int32_t packetSize = 0;
		int32_t ret = 0;   // force a read the first time
		uint16_t messageType;
		int temct;
		len = 0;

	/* read the header data from the file. Note packet header and individual message header are the same. */
		while((iReadLen=(int32_t)fread(&packetHeader ,1 , sizeof(ArcaL2Header_t) ,fptrIn)) > 0)
		{
		  // memcpy(&packetHeader, &msg.Header, sizeof(ArcaL2Header_t));

		  /* should check against a list of known message types */
		  /* bypass heart beat etc */
printf("size  %d\n",ntohs(packetHeader.iMsgSize));
printf("type  %d\n",ntohs(packetHeader.iMsgType));
printf("MsgSeqNum %d\n",ntohl(packetHeader.iMsgSeqNum));
printf("SendTime %d\n",ntohl(packetHeader.iSendTime));
printf("ProductID %d\n", packetHeader.iProductID); 
printf("RetransFlag %d\n", packetHeader.iRetransFlag); 
printf("NumBodyEntries %d\n", packetHeader.iNumBodyEntries); 
if (  packetHeader.iNumBodyEntries > 1) {
printf("case ***\n"); // break;
}
		messageType=ntohs(packetHeader.iMsgType);
		/* heartbeat is just header which is not compressed so just output */
		if(ntohs(packetHeader.iMsgType)==2)
		{
			unpackSize += sizeof(ArcaL2Header_t);
			fwrite(&packetHeader ,1,  sizeof(ArcaL2Header_t) ,fptrOut);
printf("hb so just write header pktsize =%d\n",ntohs(packetHeader.iMsgSize));
			packetSize = 0;
			iBufLen = 0;
			iBufPos = 0;

			continue;
		}

		/* check packet header to get message length */
		/* checks if message type valid  ...do we need to do this ****/
		if ( ntohs(packetHeader.iMsgType)>120 )
		{
			// seek back to where we need to be
			fseek(fptrIn,-1*((int)sizeof(ArcaL2Header_t)-1),SEEK_CUR);
			fprintf(stderr,"\n\'%c\' Unknown Message Type for Decoding (%d)",packetHeader.iMsgType,iCount);

			/* can read message length to bypass or break */		
			//iReadLen=(int32_t)fread(fastBuf,1,ntohs(packetHeader.iMsgSize), fptrIn);
			break;
		}

		/* todo We have message type M so messages are encoded */
		unpackSize += sizeof(ArcaL2Header_t);
		packetSize = ntohs(packetHeader.iMsgSize);
		packetSize+=2;// doesnt contain first two bytes
		iBufLen = sizeof(msg.Header); 
		iBufPos = 0;

		memcpy(&state,fastStateInit,sizeof(fastStateInit));
		/* read message body */
		if((iReadLen=(int32_t)fread(fastBuf,1,packetSize-sizeof(ArcaL2Header_t),fptrIn)) <= 0)
		{
			fprintf(stderr,"Read error...");
			break;
		}
printf("iReadLen = %d encoded message body \n",iReadLen);
for (temct=0;temct<iReadLen;temct++)printf("mb=%x ",fastBuf[temct]);
printf("-----------\n");
		iBufLen = iReadLen;// length of body 
		tmpLen = 0; 

		/* process all the message body from the packet data */
		while (iBufLen > 0)
		{
			len = iBufLen;

            clkStatus =
                clock_gettime( CLOCK_THREAD_CPUTIME_ID, & entryCpuClock );
            memset(&msg,0,sizeof(msg));                        /* tcs 6/12/08 */
            ret = ABFastDecode(&msg,fastBuf+iBufPos,&len,&messageType,state);
            clkStatus =
                clock_gettime( CLOCK_THREAD_CPUTIME_ID, & exitCpuClock );
            cumCpuClock .tv_sec  +=
                exitCpuClock .tv_sec  - entryCpuClock .tv_sec;
            cumCpuClock .tv_nsec +=
                exitCpuClock .tv_nsec - entryCpuClock .tv_nsec;
            if ( cumCpuClock .tv_nsec > 1000000000 ) {
                cumCpuClock .tv_nsec -= 1000000000 ;
                cumCpuClock .tv_sec  += 1 ;
            }

			// we should have already read whole packet into buffer. If we get here there is a problem with Data.
			if(ret == AB_INCOMPLETE_ERROR)
			{
				fprintf(stderr,"Incomplete Message Error...");
				break;
			}
			else if(ret == AB_OK)
			{
				
				packSize += len;

switch ( messageType )
{
	case 100: packetHeader.iMsgSize = sizeof( ArcaL2Add_t );break;
	case 101: packetHeader.iMsgSize = sizeof( ArcaL2Modify_t );break;
	case 102: packetHeader.iMsgSize = sizeof( ArcaL2Delete_t );break;
	case 103: packetHeader.iMsgSize = sizeof( ArcaL2Imbalance_t );break;
	case 35: packetHeader.iMsgSize = sizeof( ArcaL2SymbolUpdate_t );break;
	case 36: packetHeader.iMsgSize = sizeof( ArcaL2SymbolClear_t );break;
	case 37: packetHeader.iMsgSize = sizeof( ArcaL2FirmUpdate_t );break;
	case 32: packetHeader.iMsgSize = sizeof( ArcaL2BookOrder_t );break;
	case 1: packetHeader.iMsgSize = sizeof( ArcaL2SequenceReset_t );break;
	default:
printf("what is this ...\n");
	break;
}
printf("messageType=%d packetHeader.iMsgSize=%d\n",messageType,packetHeader.iMsgSize);
				unpackSize += packetHeader.iMsgSize;
				// store message in temp buffer because unknown decode length of complete packet
				memcpy(&tmpBuf[tmpLen],&msg,packetHeader.iMsgSize);
				tmpLen+=packetHeader.iMsgSize;

				iCount++;

				if(0==(iCount % 100000))
				fprintf(stderr,"\rProcessing Message %d",iCount);
				
				// advance read pointer and reduce bufffer length
				iBufPos += len;
				iBufLen -= len;
                        }
			else 
			{
				iCount++;
				fprintf(stderr,"ABFastDecode() FAILED %d on message %d\n",ret,iCount);
				break;
			}
                }// end of while


			if (tmpLen > 0)
			{
				packetHeader.iMsgSize = htons(tmpLen+sizeof(ArcaL2Header_t)-2);
				printf("ph msize %d\n",ntohs(packetHeader.iMsgSize ));
				fwrite(&packetHeader,1,sizeof(ArcaL2Header_t),fptrOut);
			   printf("writes body %d bytes \n",tmpLen);
				fwrite(tmpBuf,1,tmpLen,fptrOut);
				tmpLen = 0; 
			}
		}

		/*if (tmpLen > 0)
		{
			packetHeader.iMsgSize = htons(tmpLen+sizeof(ArcaL2Header_t)-2);
			fwrite(&packetHeader,1,sizeof(ArcaL2Header_t),fptrOut);
			fwrite(tmpBuf,1,tmpLen,fptrOut);
			tmpLen = 0; 
		}*/
	}

	//end_cpu_time = get_cpu_time();
	/* gather some stats */
	{
		double avgUnpackSize = (double)unpackSize/(double)iCount;
		double avgPackSize   = (double)packSize/(double)iCount;


		// float total_cpu = (float)(end_cpu_time - start_cpu_time);
		// float msg_cpu = total_cpu / iCount;
		// float msg_sec = iCount / (total_cpu * 1E-6f);

		fprintf(stderr,"\rProcessed %d Messages, Avg %f bytes from %f bytes (%.2f%%)",
			iCount,avgPackSize,avgUnpackSize,
			100.0*(1.0 - (double)packSize/(double)unpackSize));

		fprintf (stderr, "\nFAST decode cpu %d.%09u secs\n",
					(unsigned int)cumCpuClock .tv_sec, (unsigned int)cumCpuClock .tv_nsec);

	}

	fclose(fptrIn);
	fclose(fptrOut);

	return 0;
}

