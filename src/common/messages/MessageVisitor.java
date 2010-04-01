package common.messages;

import common.messages.MessageFactory.CAS;
import common.messages.MessageFactory.Load;
import common.messages.MessageFactory.Read;
import common.messages.MessageFactory.Result;
import common.messages.MessageFactory.Run;
import common.messages.MessageFactory.Write;

public interface MessageVisitor<C> {
	public Result visit( Read read,   C context);
	public Result visit( Write write, C context);
	public Result visit( CAS cas,     C context);
	public Result visit( Run run,     C context);
	public Result visit( Load load,   C context);
}
