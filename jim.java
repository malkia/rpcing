package beam;

import static service.Service.CommandRequest;

public class jim {
    public static void main(String args[]) {
        CommandRequest commandRequest = CommandRequest.newBuilder()
                .setCallIndex(1)
                .setCommandId(10)
                .setConnectionIndex(400)
                .build();
        System.out.printf("Whiskey in the jar: %s\n", commandRequest);
    }
}