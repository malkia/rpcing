package beam;

import java.util.logging.Logger;
import io.grpc.Server;
import static service.Service.CommandRequest;

public class jim {
    private static final Logger logger = Logger.getLogger(jim.class.getName());

    private final Server server;

    public jim()
    {

    }

    public static void main(String args[]) {
        CommandRequest commandRequest = CommandRequest.newBuilder()
                .setCallIndex(1)
                .setCommandId(10)
                .setConnectionIndex(400)
                .build();
        System.out.printf("Whiskey in the jar: %s\n", commandRequest);
    }
}