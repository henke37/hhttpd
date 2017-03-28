#ifndef REPLY_H
#define REPLY_H

struct request;

void sendReply(struct request *);
char const *replyDescFromCode(unsigned int);
void setReplyHeader(struct request *,char *name,char *value);
struct headerListNode *findReplyHeader(struct request *request,const char *name);

#endif
