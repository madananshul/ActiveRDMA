package common.messages;

import common.messages.MessageFactory.CAS;
import common.messages.MessageFactory.Load;
import common.messages.MessageFactory.Read;
import common.messages.MessageFactory.Run;
import common.messages.MessageFactory.Write;

public interface MessageVisitor<C> {
	public int visit( Read read,   C context);
	public int visit( Write write, C context);
	public int visit( CAS cas,     C context);
	public int visit( Run run,     C context);
	public int visit( Load load,   C context);
}
