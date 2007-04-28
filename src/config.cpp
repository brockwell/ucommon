#include <config.h>
#include <ucommon/string.h>
#include <ucommon/misc.h>
#include <ctype.h>

using namespace UCOMMON_NAMESPACE;

OrderedIndex keyconfig::callback::list;
SharedLock keyconfig::lock;
keyconfig *keyconfig::cfg = NULL;

keyconfig::callback::callback() :
OrderedObject(&list)
{
}

keyconfig::callback::~callback()
{
	release();
}

void keyconfig::callback::release(void)
{
	delist(&list);
}

void keyconfig::callback::reload(keyconfig *keys)
{
}

void keyconfig::callback::commit(keyconfig *keys)
{
}

keyconfig::instance::instance()
{
	keyconfig::protect();
}

keyconfig::instance::~instance()
{
	keyconfig::release();
}

keyconfig::keyconfig(char *name, size_t s) :
mempager(s), root()
{
	root.setId(name);
}

keyconfig::~keyconfig()
{
	mempager::purge();
}

keyconfig::keynode *keyconfig::getPath(const char *id)
{
	const char *np;
	char buf[65];
	char *ep;
	keynode *node = &root, *child;
	caddr_t mp;

	while(id && *id && node) {
		cpr_strset(buf, sizeof(buf), id);
		ep = strchr(buf, '.');
		if(ep)
			*ep = 0;
		np = strchr(id, '.');
		if(np)
			id = ++np;
		else
			id = NULL;
		child = node->getChild(buf);
		if(!child) {
			mp = (caddr_t)alloc(sizeof(keynode));
			child = new(mp) keynode(node, dup(buf));
		}
		node = child;			
	}
	return node;
}

keyconfig::keynode *keyconfig::addNode(keynode *base, const char *id, const char *value)
{
	caddr_t mp;
	keynode *node;

	mp = (caddr_t)alloc(sizeof(keynode));
	node = new(mp) keynode(base, dup(id));
	if(value)
		node->setPointer(dup(value));
	return node;
}

const char *keyconfig::getValue(keynode *node, const char *id, keynode *alt)
{
	node = node->getChild(id);
	if(!node && alt)
		node = alt->getChild(id);

	if(!node)
		return NULL;

	return node->getPointer();
}

keyconfig::keynode *keyconfig::addNode(keynode *base, define *defs)
{
	keynode *node = getNode(base, defs->key, defs->value);
	if(!node)
		node = addNode(base, defs->key, defs->value);

	for(;;) {
		++defs;
		if(!defs->key)
			return base;
		if(node->getChild(defs->key))
			continue;
		addNode(node, defs->key, defs->value);
	}
	return node;
}


keyconfig::keynode *keyconfig::getNode(keynode *base, const char *id, const char *text)
{
	linked_pointer<keynode> node = base->getFirst();
	char *cp;
	
	while(node) {
		if(!strcmp(id, node->getId())) {
			cp = node->getData();
			if(cp && !cpr_stricmp(cp, text))
				return *node;
		}
		node.next();
	}
	return NULL;
} 

bool keyconfig::loadconf(const char *fn, keynode *node, char *gid, keynode *entry)
{
	FILE *fp = fopen(fn, "r");
	bool rtn = false;
	keynode *data;
	caddr_t mp;
	const char *cp;
	char *value;

	if(!node)
		node = &root;

	if(!fp)
		return false;

	while(!feof(fp)) {
		buffer << fp;
		buffer.strip(" \t\r\n");
		if(buffer[0] == '[') {
			if(!buffer.unquote("[]"))
				goto exit;
			value = mempager::dup(*buffer);
			if(gid)
				entry = getNode(node, gid, value);
			else
				entry = node->getChild(value);
			if(!entry) {
				mp = (caddr_t)alloc(sizeof(keynode));
				if(gid) {
					entry = new(mp) keynode(node, gid);
					entry->setPointer(value);
				}
				else					
					entry = new(mp) keynode(node, value);
			}
		}
		if(!buffer[0] || !isalnum(buffer[0]))
			continue;	
		if(!entry)
			continue;
		cp = buffer.chr('=');
		if(!cp)
			continue;
		buffer.split(cp++);
		buffer.chop(" \t=");
		while(isspace(*cp))
			++cp;
		data = entry->getChild(buffer.c_mem());
		if(!data) {
			mp = (caddr_t)alloc(sizeof(keynode));
			data = new(mp) keynode(entry, mempager::dup(*buffer));
		}
		data->setPointer(mempager::dup(cp));
	}

	rtn = true;
exit:
	fclose(fp);
	return rtn;
}

bool keyconfig::loadxml(const char *fn, keynode *node)
{
	FILE *fp = fopen(fn, "r");
	char *cp, *ep, *bp, *id;
	ssize_t len = 0;
	bool rtn = false;
	bool document = false, empty;
	keynode *match;

	if(!node)
		node = &root;

	if(!fp)
		return false;

	buffer = "";

	while(node) {
		cp = buffer.c_mem() + buffer.len();
		if(buffer.len() < 1024 - 5) {
			len = fread(cp, 1, 1024 - buffer.len() - 1, fp);
		}
		else
			len = 0;

		if(len < 0)
			goto exit;
		cp[len] = 0;
		if(!buffer.chr('<'))
			goto exit;
		buffer = buffer.c_str();
		cp = buffer.c_mem();

		while(node && cp && *cp)
		{
			cp = cpr_strtrim(cp, " \t\r\n");

			if(cp && *cp && !node)
				goto exit;

			bp = strchr(cp, '<');
			ep = strchr(cp, '>');
			if(!ep && bp == cp)
				break;
			if(!bp ) {
				cp = cp + strlen(cp);
				break;
			}
			if(bp > cp) {
				if(node->getData() != NULL)
					goto exit;
				*bp = 0;
				cp = cpr_strchop(cp, " \r\n\t");
				len = strlen(cp);
				ep = (char *)alloc(len + 1);
				cpr_xmldecode(ep, len, cp);
				node->setPointer(ep);
				*bp = '<';
				cp = bp;
				continue;
			}

			empty = false;	
			*ep = 0;
			if(*(ep - 1) == '/') {
				*(ep - 1) = 0;
				empty = true;
			}
			cp = ++ep;

			if(!strncmp(bp, "</", 2)) {
				if(strcmp(bp + 2, node->getId()))
					goto exit;

				node = node->getParent();
				continue;
			}		

			++bp;

			// if comment/control field...
			if(!isalnum(*bp))
				continue;

			ep = bp;
			while(isalnum(*ep))
				++ep;

			id = NULL;
			if(isspace(*ep)) {
				*(ep++) = 0;
				id = strchr(ep, '\"');
			}
			else if(*ep)
				goto exit;

			if(id) {
				ep = strchr(id + 1, *id);
				if(!ep)
					goto exit;
				*ep = 0;
				++id;
			}

			if(!document) {
				if(strcmp(node->getId(), bp))
					goto exit;
				document = true;
				continue;
			}

			if(id)
				match = getNode(node, bp, id);
			else
				match = node->getChild(bp);
			if(match) {
				if(!id)
					match->setPointer(NULL);
				node = match;
			}
			else 
				node = addNode(node, bp, id);

			if(empty)
				node = node->getParent();
		}
		buffer = cp;
	}
	if(!node && root.getId())
		rtn = true;
exit:
	fclose(fp);
	return rtn;
}

void keyconfig::commit(void)
{
	cb = callback::list.begin();
	while(cb) {
		cb->reload(this);
		cb.next();
	}
	lock.lock();

	cb = callback::list.begin();
	while(cb) {
		cb->commit(this);
		cb.next();
	}
	cfg = this;
	lock.unlock();
}

void keyconfig::update(void)
{
	lock.lock();
	if(cb)
		cb.next();

	while(cb) {
		cb->reload(this);
		cb->commit(this);
		cb.next();
	}
	lock.unlock();
}